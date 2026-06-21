struct cbGen {
	gbFile *     f;
	Checker *    checker;
	bool         failed;
	bool         has_return;
	i32          proc_result_count;
	i32          temp_counter;
	PtrSet<Entity *> foreign_procs;
	PtrMap<Type *, String> emitted_type_names;
	PtrSet<Type *> emitted_type_defs;
	PtrSet<Type *> pending_type_defs;
};

#define CB_ODIN_USER_MAIN_NAME "odin_user_main"

#define CB_FAIL(g, ...) do { \
	gb_printf_err(__VA_ARGS__); \
	(g)->failed = true; \
} while (0)

gb_internal bool cb_validate_build_settings(void) {
	if (build_context.backend_kind != BackendKind_Plan9_C) {
		return true;
	}

	if (build_context.metrics.os != TargetOs_plan9) {
		gb_printf_err("-backend:plan9-c requires -target:plan9_amd64 or -target:plan9_arm64\n");
		return false;
	}

	switch (build_context.build_mode) {
	case BuildMode_Executable:
		break;
	default:
		gb_printf_err("-backend:plan9-c currently only supports -build-mode:exe\n");
		return false;
	}

	if (build_context.command_kind == Command_test) {
		gb_printf_err("-backend:plan9-c does not support `odin test` yet\n");
		return false;
	}

	return true;
}

gb_internal void cb_indent(cbGen *g, int levels) {
	for (int i = 0; i < levels; i++) {
		gb_fprintf(g->f, "\t");
	}
}

gb_internal String cb_type_c_name(cbGen *g, Type *type);
gb_internal void cb_emit_type_def(cbGen *g, Type *type);
gb_internal void cb_emit_type_defs_for_type(cbGen *g, Type *type);

gb_internal bool cb_type_to_c(cbGen *g, Type *type, gbString *out) {
	if (type == nullptr) {
		return false;
	}

	switch (type->kind) {
	case Type_Named:
		cb_emit_type_defs_for_type(g, type->Named.base);
		{
			String name = cb_type_c_name(g, type);
			*out = gb_string_append_length(*out, name.text, name.len);
		}
		return true;
	default:
		break;
	}

	type = base_type(type);
	if (type == nullptr) {
		return false;
	}

	switch (type->kind) {
	case Type_Basic:
		switch (type->Basic.kind) {
		case Basic_bool:  *out = gb_string_appendc(*out, "int"); break;
		case Basic_i8:    *out = gb_string_appendc(*out, "signed char"); break;
		case Basic_u8:    *out = gb_string_appendc(*out, "unsigned char"); break;
		case Basic_i16:   *out = gb_string_appendc(*out, "short"); break;
		case Basic_u16:   *out = gb_string_appendc(*out, "unsigned short"); break;
		case Basic_i32:   *out = gb_string_appendc(*out, "int"); break;
		case Basic_u32:   *out = gb_string_appendc(*out, "unsigned int"); break;
		case Basic_i64:   *out = gb_string_appendc(*out, "long long"); break;
		case Basic_u64:   *out = gb_string_appendc(*out, "unsigned long long"); break;
		case Basic_i128:  *out = gb_string_appendc(*out, "long long"); break;
		case Basic_u128:  *out = gb_string_appendc(*out, "unsigned long long"); break;
		case Basic_rune:  *out = gb_string_appendc(*out, "int"); break;
		case Basic_f32:   *out = gb_string_appendc(*out, "float"); break;
		case Basic_f64:   *out = gb_string_appendc(*out, "double"); break;
		case Basic_int:   *out = gb_string_appendc(*out, "int"); break;
		case Basic_uint:  *out = gb_string_appendc(*out, "unsigned int"); break;
		case Basic_uintptr:*out = gb_string_appendc(*out, "unsigned long"); break;
		case Basic_cstring:*out = gb_string_appendc(*out, "char"); break;
		case Basic_string:
			cb_emit_type_defs_for_type(g, type);
			*out = gb_string_appendc(*out, "odin_string");
			break;
		case Basic_rawptr: *out = gb_string_appendc(*out, "void"); break;
		case Basic_any:
			*out = gb_string_appendc(*out, "odin_any");
			break;
		case Basic_typeid:
			*out = gb_string_appendc(*out, "unsigned long long");
			break;
		default:
			CB_FAIL(g, "Plan 9 C backend: unsupported basic type %.*s\n", LIT(type->Basic.name));
			return false;
		}
		break;
	case Type_Pointer:
		if (!cb_type_to_c(g, type->Pointer.elem, out)) {
			return false;
		}
		*out = gb_string_appendc(*out, "*");
		break;
	case Type_MultiPointer:
		if (!cb_type_to_c(g, type->MultiPointer.elem, out)) {
			return false;
		}
		*out = gb_string_appendc(*out, "*");
		break;
	case Type_Struct:
	case Type_Enum:
	case Type_Slice:
		cb_emit_type_defs_for_type(g, type);
		{
			String name = cb_type_c_name(g, type);
			*out = gb_string_append_length(*out, name.text, name.len);
		}
		break;
	case Type_Array:
		if (!cb_type_to_c(g, type->Array.elem, out)) {
			return false;
		}
		break;
	case Type_BitSet:
		{
			Type *backing = bit_set_to_int(type);
			if (!cb_type_to_c(g, backing, out)) {
				return false;
			}
		}
		break;
	case Type_Union:
	case Type_Map:
	case Type_DynamicArray:
	case Type_SimdVector:
		cb_emit_type_defs_for_type(g, type);
		{
			String name = cb_type_c_name(g, type);
			*out = gb_string_append_length(*out, name.text, name.len);
		}
		break;
	default:
		CB_FAIL(g, "Plan 9 C backend: unsupported type %s\n", type_to_string(type));
		return false;
	}
	return true;
}

gb_internal void cb_emit_c_var(cbGen *g, Type *type, String name) {
	Type *orig = type;
	type = base_type(type);

	if (orig != nullptr && orig->kind == Type_Pointer) {
		Type *elem = base_type(orig->Pointer.elem);
		if (elem != nullptr && elem->kind == Type_Array) {
			gbString elem_c = gb_string_make(temporary_allocator(), "");
			if (!cb_type_to_c(g, elem->Array.elem, &elem_c)) {
				return;
			}
			gb_fprintf(g->f, "%s *%.*s", elem_c, LIT(name));
			return;
		}
	}

	if (type != nullptr && type->kind == Type_Array) {
		gbString elem_c = gb_string_make(temporary_allocator(), "");
		if (!cb_type_to_c(g, type->Array.elem, &elem_c)) {
			return;
		}
		gb_fprintf(g->f, "%s %.*s[%lld]", elem_c, LIT(name), cast(long long)type->Array.count);
		return;
	}

	gbString type_c = gb_string_make(temporary_allocator(), "");
	if (!cb_type_to_c(g, orig, &type_c)) {
		return;
	}
	if (is_type_cstring(orig)) {
		gb_fprintf(g->f, "%s *%.*s", type_c, LIT(name));
	} else {
		gb_fprintf(g->f, "%s %.*s", type_c, LIT(name));
	}
}

gb_internal String cb_entity_link_name(Entity *e) {
	if (e == nullptr) {
		return str_lit("");
	}
	if (e->kind == Entity_Procedure && e->Procedure.link_name.len > 0) {
		return e->Procedure.link_name;
	}
	return e->token.string;
}

gb_internal void cb_note_foreign_proc(cbGen *g, Entity *e) {
	if (e != nullptr && e->kind == Entity_Procedure && e->Procedure.is_foreign) {
		ptr_set_add(&g->foreign_procs, e);
	}
}

gb_internal bool cb_is_runtime_foreign(String name) {
	/* Declared in odin_generated.h via libodin_plan9 headers. */
	return string_starts_with(name, str_lit("sys_"));
}

gb_internal bool cb_wants_runtime_emission(void) {
	char const *key = string_intern_cstring(str_lit("ODIN_PLAN9_EMIT_RUNTIME"));
	ExactValue *v = map_get(&build_context.defined_values, key);
	return v != nullptr && v->kind == ExactValue_Bool && v->value_bool;
}

gb_internal bool cb_should_skip_compiler_proc(Entity *e) {
	if (e == nullptr || e->kind != Entity_Procedure) {
		return true;
	}
	String link = cb_entity_link_name(e);
	if (link.len > 0 && link.text[0] == '_' && link.len > 1 && link.text[1] == '_') {
		return true;
	}
	return false;
}

gb_internal bool cb_is_emittable_package(AstPackage *pkg, CheckerInfo *info) {
	if (pkg == nullptr || info == nullptr) {
		return false;
	}
	if (pkg == info->builtin_package || pkg == intrinsics_pkg) {
		return false;
	}
	if (pkg == info->runtime_package && !cb_wants_runtime_emission()) {
		return false;
	}
	return true;
}

gb_internal String cb_sanitize_c_name(String name) {
	gbString out = gb_string_make(temporary_allocator(), "odin_");
	for (isize i = 0; i < name.len; i++) {
		u8 c = name.text[i];
		char ch[2] = {(char)c, 0};
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') {
			out = gb_string_appendc(out, ch);
		} else {
			out = gb_string_appendc(out, "_");
		}
	}
	return make_string((u8 *)out, gb_string_length(out));
}

gb_internal String cb_type_c_name(cbGen *g, Type *type) {
	type = base_type(type);
	if (type == nullptr) {
		return str_lit("void");
	}

	if (is_type_string(type)) {
		return str_lit("odin_string");
	}

	if (is_type_any(type)) {
		return str_lit("odin_any");
	}

	String *found = map_get(&g->emitted_type_names, type);
	if (found != nullptr) {
		return *found;
	}

	String name = {};
	if (type->kind == Type_Named && type->Named.type_name != nullptr) {
		name = cb_sanitize_c_name(type->Named.type_name->token.string);
	} else {
		gbString anon = gb_string_make(temporary_allocator(), "odin_t_");
		anon = gb_string_append_fmt(anon, "%llu", cast(unsigned long long)cast(uintptr)type);
		name = make_string_c(anon);
	}

	map_set(&g->emitted_type_names, type, name);
	return name;
}

gb_internal void cb_note_type_for_decl(cbGen *g, Type *type) {
	if (type == nullptr || g->failed) {
		return;
	}
	type = base_type(type);
	if (type == nullptr || ptr_set_exists(&g->emitted_type_defs, type)) {
		return;
	}
	if (ptr_set_exists(&g->pending_type_defs, type)) {
		return;
	}
	ptr_set_add(&g->pending_type_defs, type);
}

gb_internal void cb_emit_pending_type_defs(cbGen *g) {
	Type *type = nullptr;
	FOR_PTR_SET(type, g->pending_type_defs) {
		cb_emit_type_def(g, type);
		if (g->failed) {
			return;
		}
	}
}

gb_internal void cb_collect_types_from_expr(cbGen *g, Ast *expr);
gb_internal void cb_collect_types_from_stmt(cbGen *g, Ast *stmt);

gb_internal void cb_collect_types_from_expr(cbGen *g, Ast *expr) {
	if (expr == nullptr || g->failed) {
		return;
	}

	expr = unparen_expr(expr);
	if (expr->kind == Ast_SliceExpr) {
		cb_note_type_for_decl(g, type_of_expr(expr));
	}

	switch (expr->kind) {
	case Ast_SliceExpr:
		cb_collect_types_from_expr(g, expr->SliceExpr.expr);
		cb_collect_types_from_expr(g, expr->SliceExpr.low);
		cb_collect_types_from_expr(g, expr->SliceExpr.high);
		break;
	case Ast_CompoundLit:
		cb_note_type_for_decl(g, type_of_expr(expr));
		for (Ast *elem : expr->CompoundLit.elems) {
			cb_collect_types_from_expr(g, elem);
		}
		break;
	case Ast_BinaryExpr:
		cb_collect_types_from_expr(g, expr->BinaryExpr.left);
		cb_collect_types_from_expr(g, expr->BinaryExpr.right);
		break;
	case Ast_UnaryExpr:
		cb_collect_types_from_expr(g, expr->UnaryExpr.expr);
		break;
	case Ast_DerefExpr:
		cb_collect_types_from_expr(g, expr->DerefExpr.expr);
		break;
	case Ast_CallExpr:
		cb_collect_types_from_expr(g, expr->CallExpr.proc);
		for (Ast *arg : expr->CallExpr.args) {
			cb_collect_types_from_expr(g, arg);
		}
		break;
	case Ast_IndexExpr:
		cb_collect_types_from_expr(g, expr->IndexExpr.expr);
		cb_collect_types_from_expr(g, expr->IndexExpr.index);
		break;
	case Ast_SelectorExpr:
		cb_collect_types_from_expr(g, expr->SelectorExpr.expr);
		break;
	case Ast_TypeCast:
		cb_collect_types_from_expr(g, expr->TypeCast.expr);
		break;
	default:
		break;
	}
}

gb_internal void cb_collect_types_from_stmt(cbGen *g, Ast *stmt) {
	if (stmt == nullptr || g->failed) {
		return;
	}

	switch (stmt->kind) {
	case Ast_BlockStmt:
		for (Ast *s : stmt->BlockStmt.stmts) {
			cb_collect_types_from_stmt(g, s);
		}
		break;
	case Ast_AssignStmt:
		for (Ast *lhs : stmt->AssignStmt.lhs) {
			cb_collect_types_from_expr(g, lhs);
		}
		for (Ast *rhs : stmt->AssignStmt.rhs) {
			cb_collect_types_from_expr(g, rhs);
		}
		break;
	case Ast_ExprStmt:
		cb_collect_types_from_expr(g, stmt->ExprStmt.expr);
		break;
	case Ast_ReturnStmt:
		for (Ast *r : stmt->ReturnStmt.results) {
			cb_collect_types_from_expr(g, r);
		}
		break;
	case Ast_IfStmt:
		cb_collect_types_from_stmt(g, stmt->IfStmt.init);
		cb_collect_types_from_expr(g, stmt->IfStmt.cond);
		cb_collect_types_from_stmt(g, stmt->IfStmt.body);
		cb_collect_types_from_stmt(g, stmt->IfStmt.else_stmt);
		break;
	case Ast_ForStmt:
		cb_collect_types_from_stmt(g, stmt->ForStmt.init);
		cb_collect_types_from_expr(g, stmt->ForStmt.cond);
		cb_collect_types_from_stmt(g, stmt->ForStmt.post);
		cb_collect_types_from_stmt(g, stmt->ForStmt.body);
		break;
	case Ast_RangeStmt:
		cb_collect_types_from_stmt(g, stmt->RangeStmt.init);
		cb_collect_types_from_expr(g, stmt->RangeStmt.expr);
		cb_collect_types_from_stmt(g, stmt->RangeStmt.body);
		break;
	case Ast_SwitchStmt:
		cb_collect_types_from_stmt(g, stmt->SwitchStmt.init);
		cb_collect_types_from_expr(g, stmt->SwitchStmt.tag);
		cb_collect_types_from_stmt(g, stmt->SwitchStmt.body);
		break;
	case Ast_ValueDecl:
		for (Ast *v : stmt->ValueDecl.values) {
			cb_collect_types_from_expr(g, v);
		}
		break;
	default:
		break;
	}
}

gb_internal void cb_collect_types_from_checker(cbGen *g, Checker *checker) {
	CheckerInfo *info = &checker->info;
	for (isize i = 0; i < info->entities.count; i++) {
		Entity *e = info->entities[i];
		if (e->kind != Entity_Procedure || e->Procedure.is_foreign) {
			continue;
		}
		if (!cb_is_emittable_package(e->pkg, info)) {
			continue;
		}
		if (e->min_dep_count.load(std::memory_order_relaxed) == 0) {
			continue;
		}
		if (cb_should_skip_compiler_proc(e)) {
			continue;
		}
		DeclInfo *decl = decl_info_of_entity(e);
		if (decl == nullptr || decl->proc_lit == nullptr) {
			continue;
		}
		ast_node(pl, ProcLit, decl->proc_lit);
		cb_collect_types_from_stmt(g, pl->body);
	}
}

gb_internal void cb_emit_type_defs_for_type(cbGen *g, Type *type) {
	if (type == nullptr || g->failed) {
		return;
	}
	type = base_type(type);

	if (is_type_string(type)) {
		if (!ptr_set_update(&g->emitted_type_defs, type)) {
			gb_fprintf(g->f, "typedef struct odin_string odin_string;\n");
			gb_fprintf(g->f, "struct odin_string {\n");
			gb_fprintf(g->f, "\tunsigned char *data;\n");
			gb_fprintf(g->f, "\tlong len;\n");
			gb_fprintf(g->f, "};\n\n");
		}
		return;
	}

	if (is_type_any(type)) {
		if (!ptr_set_update(&g->emitted_type_defs, type)) {
			gb_fprintf(g->f, "typedef struct odin_any odin_any;\n");
			gb_fprintf(g->f, "struct odin_any {\n");
			gb_fprintf(g->f, "\tvoid *data;\n");
			gb_fprintf(g->f, "\tunsigned long long id;\n");
			gb_fprintf(g->f, "};\n\n");
		}
		return;
	}

	switch (type->kind) {
	case Type_Named:
		cb_emit_type_def(g, type);
		break;
	case Type_Pointer:
		cb_emit_type_defs_for_type(g, type->Pointer.elem);
		break;
	case Type_Slice:
		cb_emit_type_def(g, type);
		break;
	case Type_Struct:
	case Type_Enum:
	case Type_Union:
	case Type_Map:
	case Type_DynamicArray:
	case Type_SimdVector:
	case Type_BitSet:
		cb_emit_type_def(g, type);
		break;
	case Type_Array:
		ptr_set_update(&g->emitted_type_defs, type);
		break;
	default:
		break;
	}
}

gb_internal void cb_emit_type_def(cbGen *g, Type *type) {
	if (type == nullptr || g->failed) {
		return;
	}
	type = base_type(type);

	if (is_type_string(type)) {
		cb_emit_type_defs_for_type(g, type);
		return;
	}

	if (ptr_set_update(&g->emitted_type_defs, type)) {
		return;
	}

	switch (type->kind) {
	case Type_Named:
		{
			if (ptr_set_update(&g->emitted_type_defs, type)) {
				return;
			}
			Type *base = base_type(type->Named.base);
			String name = cb_type_c_name(g, type);
			if (base != nullptr) {
				map_set(&g->emitted_type_names, base, name);
			}
			if (base != nullptr && base->kind == Type_BitSet) {
				gbString backing_c = gb_string_make(temporary_allocator(), "");
				if (!cb_type_to_c(g, bit_set_to_int(base), &backing_c)) {
					return;
				}
				gb_fprintf(g->f, "typedef %s %.*s;\n\n", backing_c, LIT(name));
				return;
			}
			cb_emit_type_def(g, base);
		}
		return;

	case Type_BitSet:
		{
			gbString backing_c = gb_string_make(temporary_allocator(), "");
			if (!cb_type_to_c(g, bit_set_to_int(type), &backing_c)) {
				return;
			}
			String name = cb_type_c_name(g, type);
			gb_fprintf(g->f, "typedef %s %.*s;\n\n", backing_c, LIT(name));
		}
		break;

	case Type_Union:
		{
			type_set_offsets(type);
			String name = cb_type_c_name(g, type);
			i64 block_size = type->Union.variant_block_size;
			i64 tag_size = union_tag_size(type);
			gb_fprintf(g->f, "typedef struct %.*s %.*s;\n", LIT(name), LIT(name));
			gb_fprintf(g->f, "struct %.*s {\n", LIT(name));
			gb_fprintf(g->f, "\tunsigned char _variants[%lld];\n", cast(long long)block_size);
			if (tag_size > 0) {
				gbString tag_c = gb_string_make(temporary_allocator(), "");
				if (!cb_type_to_c(g, union_tag_type(type), &tag_c)) {
					return;
				}
				gb_fprintf(g->f, "\t%s tag;\n", tag_c);
			}
			gb_fprintf(g->f, "};\n\n");
		}
		break;

	case Type_DynamicArray:
		{
			Type *elem = type->DynamicArray.elem;
			cb_emit_type_defs_for_type(g, elem);
			String name = cb_type_c_name(g, type);
			String elem_name = cb_type_c_name(g, elem);
			gb_fprintf(g->f, "typedef struct %.*s %.*s;\n", LIT(name), LIT(name));
			gb_fprintf(g->f, "struct %.*s {\n", LIT(name));
			gb_fprintf(g->f, "\t%.*s *data;\n", LIT(elem_name));
			gb_fprintf(g->f, "\tlong len;\n");
			gb_fprintf(g->f, "\tlong cap;\n");
			gb_fprintf(g->f, "\tunsigned long long allocator;\n");
			gb_fprintf(g->f, "};\n\n");
		}
		break;

	case Type_Map:
		{
			String name = cb_type_c_name(g, type);
			gb_fprintf(g->f, "typedef struct %.*s %.*s;\n", LIT(name), LIT(name));
			gb_fprintf(g->f, "struct %.*s {\n", LIT(name));
			gb_fprintf(g->f, "\tvoid *data;\n");
			gb_fprintf(g->f, "\tlong len;\n");
			gb_fprintf(g->f, "\tlong cap;\n");
			gb_fprintf(g->f, "\tunsigned long long allocator;\n");
			gb_fprintf(g->f, "};\n\n");
		}
		break;

	case Type_SimdVector:
		{
			Type *elem = type->SimdVector.elem;
			cb_emit_type_defs_for_type(g, elem);
			String name = cb_type_c_name(g, type);
			String elem_name = cb_type_c_name(g, elem);
			gb_fprintf(g->f, "typedef struct %.*s %.*s;\n", LIT(name), LIT(name));
			gb_fprintf(g->f, "struct %.*s {\n", LIT(name));
			gb_fprintf(g->f, "\t%.*s data[%lld];\n", LIT(elem_name), cast(long long)type->SimdVector.count);
			gb_fprintf(g->f, "};\n\n");
		}
		break;

	case Type_Enum:
		{
			String name = cb_type_c_name(g, type);
			gb_fprintf(g->f, "typedef int %.*s;\n\n", LIT(name));
		}
		break;

	case Type_Array:
		ptr_set_update(&g->emitted_type_defs, type);
		break;

	case Type_Slice:
		{
			Type *elem = type->Slice.elem;
			cb_emit_type_defs_for_type(g, elem);
			String name = cb_type_c_name(g, type);
			gbString elem_c = gb_string_make(temporary_allocator(), "");
			if (!cb_type_to_c(g, elem, &elem_c)) {
				return;
			}
			gb_fprintf(g->f, "typedef struct %.*s %.*s;\n", LIT(name), LIT(name));
			gb_fprintf(g->f, "struct %.*s {\n", LIT(name));
			gb_fprintf(g->f, "\t%s *data;\n", elem_c);
			gb_fprintf(g->f, "\tlong len;\n");
			gb_fprintf(g->f, "\tlong cap;\n");
			gb_fprintf(g->f, "};\n\n");
		}
		break;

	case Type_Struct:
		{
			type_set_offsets(type);
			String name = cb_type_c_name(g, type);

			for_array(i, type->Struct.fields) {
				cb_emit_type_defs_for_type(g, type->Struct.fields[i]->type);
			}

			gb_fprintf(g->f, "typedef struct %.*s %.*s;\n", LIT(name), LIT(name));
			gb_fprintf(g->f, "struct %.*s {\n", LIT(name));

			i64 prev_offset = 0;
			for_array(i, type->Struct.fields) {
				Entity *field = type->Struct.fields[i];
				i64 offset = type->Struct.offsets[i];
				i64 padding = offset - prev_offset;
				if (padding > 0) {
					gb_fprintf(g->f, "\tunsigned char _pad%lld[%lld];\n", cast(long long)i, cast(long long)padding);
				}

				gbString field_type = gb_string_make(temporary_allocator(), "");
				if (!cb_type_to_c(g, field->type, &field_type)) {
					return;
				}
				gb_fprintf(g->f, "\t%s %.*s;\n", field_type, LIT(field->token.string));
				prev_offset = offset + type_size_of(field->type);
			}

			i64 full_size = type_size_of(type);
			i64 end_padding = full_size - prev_offset;
			if (end_padding > 0) {
				gb_fprintf(g->f, "\tunsigned char _endpad[%lld];\n", cast(long long)end_padding);
			}

			gb_fprintf(g->f, "};\n\n");
		}
		break;

	default:
		break;
	}
}

gb_internal void cb_emit_escaped_c_string(cbGen *g, String s) {
	gb_fprintf(g->f, "\"");
	for (isize i = 0; i < s.len; i++) {
		u8 c = s.text[i];
		switch (c) {
		case '\\': gb_fprintf(g->f, "\\\\"); break;
		case '\"': gb_fprintf(g->f, "\\\""); break;
		case '\n': gb_fprintf(g->f, "\\n"); break;
		case '\r': gb_fprintf(g->f, "\\r"); break;
		case '\t': gb_fprintf(g->f, "\\t"); break;
		default:
			if (c < 32 || c == 127) {
				gb_fprintf(g->f, "\\x%02x", c);
			} else {
				gb_fprintf(g->f, "%c", c);
			}
			break;
		}
	}
	gb_fprintf(g->f, "\"");
}

gb_internal void cb_emit_expr(cbGen *g, Ast *expr);
gb_internal void cb_emit_stmt(cbGen *g, Ast *stmt, int indent);
gb_internal void cb_emit_block(cbGen *g, Ast *block, int indent);

gb_internal bool cb_token_to_c_op(TokenKind kind, String *out) {
	switch (kind) {
	case Token_Add: case Token_Sub: case Token_Mul: case Token_Quo: case Token_Mod:
	case Token_And: case Token_Or: case Token_Xor: case Token_AndNot:
	case Token_Shl: case Token_Shr:
	case Token_CmpAnd: case Token_CmpOr:
	case Token_CmpEq: case Token_NotEq:
	case Token_Lt: case Token_Gt: case Token_LtEq: case Token_GtEq:
	case Token_AddEq: case Token_SubEq: case Token_MulEq: case Token_QuoEq: case Token_ModEq:
	case Token_AndEq: case Token_OrEq: case Token_XorEq: case Token_AndNotEq:
	case Token_ShlEq: case Token_ShrEq:
		*out = token_strings[kind];
		return true;
	default:
		return false;
	}
}

gb_internal u64 cb_typeid_u64_for_type(Type *type) {
	type = default_type(type);
	return type_hash_canonical_type(type);
}

gb_internal void cb_emit_exact_value(cbGen *g, ExactValue value) {
	switch (value.kind) {
	case ExactValue_Integer:
		{
			String s = big_int_to_string(temporary_allocator(), &value.value_integer);
			gb_fprintf(g->f, "%.*s", LIT(s));
		}
		break;
	case ExactValue_Float:
		gb_fprintf(g->f, "%g", value.value_float);
		break;
	case ExactValue_Bool:
		gb_fprintf(g->f, "%s", value.value_bool ? "1" : "0");
		break;
	case ExactValue_Typeid:
		gb_fprintf(g->f, "%llu", cast(unsigned long long)cb_typeid_u64_for_type(value.value_typeid));
		break;
	default:
		CB_FAIL(g, "Plan 9 C backend: unsupported exact value\n");
		break;
	}
}

gb_internal bool cb_call_is_odin_variadic(Entity *proc) {
	if (proc == nullptr) {
		return false;
	}
	Type *type = base_type(proc->type);
	if (type == nullptr || type->kind != Type_Proc) {
		return false;
	}
	return type->Proc.variadic && !type->Proc.c_vararg;
}

gb_internal bool cb_call_needs_variadic_pack(Ast *expr) {
	ast_node(ce, CallExpr, expr);
	Entity *proc = entity_of_node(ce->proc);
	if (proc == nullptr) {
		proc = ce->entity_procedure_of;
	}
	if (!cb_call_is_odin_variadic(proc)) {
		return false;
	}
	if (ce->split_args == nullptr) {
		return false;
	}
	if (ce->ellipsis.pos.line != 0) {
		return false;
	}
	Type *type = base_type(proc->type);
	TypeProc *pt = &type->Proc;
	isize variadic_count = ce->split_args->positional.count - pt->variadic_index;
	return variadic_count > 0;
}

gb_internal void cb_emit_call_args(cbGen *g, Ast *expr, Entity *proc, char const *slice_temp) {
	ast_node(ce, CallExpr, expr);
	Type *type = base_type(proc->type);
	TypeProc *pt = &type->Proc;

	for (isize i = 0; i < pt->param_count; i++) {
		if (i > 0) {
			gb_fprintf(g->f, ", ");
		}
		if (pt->variadic && i == pt->variadic_index) {
			gb_fprintf(g->f, "%s", slice_temp);
		} else if (i < ce->split_args->positional.count) {
			cb_emit_expr(g, ce->split_args->positional[i]);
		} else {
			CB_FAIL(g, "Plan 9 C backend: missing call argument\n");
			return;
		}
	}
}

gb_internal void cb_emit_variadic_call_stmts(cbGen *g, Ast *expr, int indent) {
	ast_node(ce, CallExpr, expr);
	Entity *proc = entity_of_node(ce->proc);
	if (proc == nullptr) {
		proc = ce->entity_procedure_of;
	}
	if (proc == nullptr) {
		CB_FAIL(g, "Plan 9 C backend: call to unknown procedure\n");
		return;
	}

	Type *type = base_type(proc->type);
	TypeProc *pt = &type->Proc;
	GB_ASSERT(pt->variadic && !pt->c_vararg);

	isize variadic_count = ce->split_args->positional.count - pt->variadic_index;
	Type *slice_type = pt->params->Tuple.variables[pt->variadic_index]->type;
	cb_emit_type_defs_for_type(g, slice_type);
	cb_emit_type_defs_for_type(g, slice_type->Slice.elem);

	String slice_name = cb_type_c_name(g, slice_type);
	String elem_name = cb_type_c_name(g, slice_type->Slice.elem);

	i32 base = g->temp_counter;
	g->temp_counter += cast(i32)variadic_count + 2;

	gbString slice_temp = gb_string_make(temporary_allocator(), "");
	slice_temp = gb_string_append_fmt(slice_temp, "_cb_sl_%d", base + cast(i32)variadic_count + 1);
	gbString arr_temp = gb_string_make(temporary_allocator(), "");
	arr_temp = gb_string_append_fmt(arr_temp, "_cb_ar_%d", base + cast(i32)variadic_count);

	cb_indent(g, indent);
	gb_fprintf(g->f, "{\n");

	if (variadic_count > 0) {
		auto value_temps = array_make<gbString>(temporary_allocator(), variadic_count);
		auto elem_typeids = array_make<u64>(temporary_allocator(), variadic_count);

		for (isize i = 0; i < variadic_count; i++) {
			Ast *arg = ce->split_args->positional[pt->variadic_index + i];
			Type *arg_type = default_type(type_of_expr(arg));
			value_temps[i] = gb_string_make(temporary_allocator(), "");
			value_temps[i] = gb_string_append_fmt(value_temps[i], "_cb_v_%d", base + cast(i32)i);
			elem_typeids[i] = cb_typeid_u64_for_type(arg_type);
			cb_indent(g, indent+1);
			cb_emit_c_var(g, arg_type, make_string_c(value_temps[i]));
			gb_fprintf(g->f, ";\n");
		}

		cb_indent(g, indent+1);
		gb_fprintf(g->f, "%.*s %s[%lld];\n", LIT(elem_name), arr_temp, cast(long long)variadic_count);
		cb_indent(g, indent+1);
		gb_fprintf(g->f, "%.*s %s;\n", LIT(slice_name), slice_temp);

		for (isize i = 0; i < variadic_count; i++) {
			Ast *arg = ce->split_args->positional[pt->variadic_index + i];
			cb_indent(g, indent+1);
			gb_fprintf(g->f, "%s = ", value_temps[i]);
			cb_emit_expr(g, arg);
			gb_fprintf(g->f, ";\n");
			cb_indent(g, indent+1);
			gb_fprintf(g->f, "%s[%lld] = ((odin_any){(void *)&%s, %llu});\n",
			           arr_temp, cast(long long)i, value_temps[i], cast(unsigned long long)elem_typeids[i]);
		}

		cb_indent(g, indent+1);
		gb_fprintf(g->f, "%s.data = %s;\n", slice_temp, arr_temp);
		cb_indent(g, indent+1);
		gb_fprintf(g->f, "%s.len = %lld;\n", slice_temp, cast(long long)variadic_count);
		cb_indent(g, indent+1);
		gb_fprintf(g->f, "%s.cap = %lld;\n", slice_temp, cast(long long)variadic_count);
	} else {
		cb_indent(g, indent+1);
		gb_fprintf(g->f, "%.*s %s;\n", LIT(slice_name), slice_temp);
		cb_indent(g, indent+1);
		gb_fprintf(g->f, "%s.data = 0;\n", slice_temp);
		cb_indent(g, indent+1);
		gb_fprintf(g->f, "%s.len = 0;\n", slice_temp);
		cb_indent(g, indent+1);
		gb_fprintf(g->f, "%s.cap = 0;\n", slice_temp);
	}

	if (proc->Procedure.is_foreign) {
		cb_note_foreign_proc(g, proc);
	}

	cb_indent(g, indent+1);
	gb_fprintf(g->f, "%.*s(", LIT(cb_entity_link_name(proc)));
	cb_emit_call_args(g, expr, proc, slice_temp);
	gb_fprintf(g->f, ");\n");

	cb_indent(g, indent);
	gb_fprintf(g->f, "}\n");
}

gb_internal void cb_emit_call_expr(cbGen *g, Ast *expr) {
	ast_node(ce, CallExpr, expr);
	Entity *proc = entity_of_node(ce->proc);
	if (proc == nullptr && ce->entity_procedure_of != nullptr) {
		proc = ce->entity_procedure_of;
	}

	if (ce->proc->tav.mode == Addressing_Type && ce->args.count == 1) {
		gbString type_c = gb_string_make(temporary_allocator(), "");
		Type *cast_type = ce->proc->tav.type;
		if (!cb_type_to_c(g, cast_type, &type_c)) {
			return;
		}
		gb_fprintf(g->f, "(%s)(", type_c);
		cb_emit_expr(g, ce->args[0]);
		gb_fprintf(g->f, ")");
		return;
	}

	if (ce->proc->tav.mode == Addressing_Builtin) {
		BuiltinProcId builtin_id = BuiltinProc_Invalid;
		if (proc != nullptr) {
			builtin_id = cast(BuiltinProcId)proc->Builtin.id;
		}
		switch (builtin_id) {
		case BuiltinProc_len:
			if (ce->args.count != 1) {
				CB_FAIL(g, "Plan 9 C backend: invalid len call\n");
				return;
			}
			{
				Type *arg_type = base_type(type_of_expr(ce->args[0]));
				if (is_type_array(arg_type)) {
					gb_fprintf(g->f, "%lld", cast(long long)get_array_type_count(arg_type));
				} else {
					gb_fprintf(g->f, "(");
					cb_emit_expr(g, ce->args[0]);
					gb_fprintf(g->f, ".len)");
				}
			}
			return;
		case BuiltinProc_raw_data:
			if (ce->args.count != 1) {
				CB_FAIL(g, "Plan 9 C backend: invalid raw_data call\n");
				return;
			}
			{
				Ast *arg = unparen_expr(ce->args[0]);
				if (arg != nullptr && arg->kind == Ast_SliceExpr) {
					ast_node(se, SliceExpr, arg);
					Type *src_type = type_of_expr(se->expr);
					Type *base = base_type(type_deref(src_type));
					gbString elem_c = gb_string_make(temporary_allocator(), "");
					Type *elem_type = base;
					if (base != nullptr && base->kind == Type_Slice) {
						elem_type = base_type(base->Slice.elem);
					} else if (base != nullptr && base->kind == Type_Array) {
						elem_type = base_type(base->Array.elem);
					}
					if (!cb_type_to_c(g, elem_type, &elem_c)) {
						return;
					}
					if (base != nullptr && base->kind == Type_Array) {
						gb_fprintf(g->f, "(%s *)&", elem_c);
						cb_emit_expr(g, se->expr);
						gb_fprintf(g->f, "[");
						if (se->low != nullptr) {
							cb_emit_expr(g, se->low);
						} else {
							gb_fprintf(g->f, "0");
						}
						gb_fprintf(g->f, "]");
					} else if (base != nullptr && base->kind == Type_Slice) {
						gb_fprintf(g->f, "(%s *)", elem_c);
						cb_emit_expr(g, se->expr);
						gb_fprintf(g->f, ".data");
						if (se->low != nullptr) {
							gb_fprintf(g->f, " + ");
							cb_emit_expr(g, se->low);
						}
					} else {
						CB_FAIL(g, "Plan 9 C backend: unsupported raw_data slice\n");
					}
					return;
				}

				Type *arg_type = type_of_expr(ce->args[0]);
				Type *elem = base_type(type_deref(arg_type));
				cb_emit_expr(g, ce->args[0]);
				if (is_type_pointer(arg_type) && elem != nullptr && elem->kind == Type_Array) {
					/* pointer to array — already a pointer in C */
				} else if (elem != nullptr && elem->kind == Type_Slice) {
					gb_fprintf(g->f, ".data");
				} else if (elem != nullptr && elem->kind == Type_Array) {
					/* native C array — decays to element pointer */
				} else {
					gb_fprintf(g->f, ".data");
				}
			}
			return;
		case BuiltinProc_typeid_of:
			if (ce->args.count != 1) {
				CB_FAIL(g, "Plan 9 C backend: invalid typeid_of call\n");
				return;
			}
			{
				Type *t = ce->args[0]->tav.type;
				if (t == nullptr) {
					t = type_of_expr(ce->args[0]);
				}
				gb_fprintf(g->f, "%llu", cast(unsigned long long)cb_typeid_u64_for_type(t));
			}
			return;
		default:
			CB_FAIL(g, "Plan 9 C backend: unsupported builtin %.*s\n", LIT(proc != nullptr ? proc->token.string : str_lit("?")));
			return;
		}
	}

	if (proc == nullptr) {
		CB_FAIL(g, "Plan 9 C backend: call to unknown procedure\n");
		return;
	}

	if (cb_call_needs_variadic_pack(expr)) {
		CB_FAIL(g, "Plan 9 C backend: variadic call must be used as a statement\n");
		return;
	}

	if (proc->Procedure.is_foreign) {
		cb_note_foreign_proc(g, proc);
	}

	String name = cb_entity_link_name(proc);
	gb_fprintf(g->f, "%.*s(", LIT(name));
	if (ce->split_args != nullptr) {
		cb_emit_call_args(g, expr, proc, "");
	} else {
		for (isize i = 0; i < ce->args.count; i++) {
			if (i > 0) {
				gb_fprintf(g->f, ", ");
			}
			cb_emit_expr(g, ce->args[i]);
		}
	}
	gb_fprintf(g->f, ")");
}

gb_internal void cb_emit_expr(cbGen *g, Ast *expr) {
	if (g->failed) {
		return;
	}

	expr = unparen_expr(expr);
	switch (expr->kind) {
	case_ast_node(bl, BasicLit, expr);
		if (bl->token.kind == Token_String) {
			if (expr->tav.value.kind == ExactValue_String) {
				String str_val = expr->tav.value.value_string;
				if (is_type_string(type_of_expr(expr))) {
					cb_emit_type_defs_for_type(g, t_string);
					gb_fprintf(g->f, "((odin_string){(unsigned char*)");
					cb_emit_escaped_c_string(g, str_val);
					gb_fprintf(g->f, ", %lld})", cast(long long)str_val.len);
				} else {
					cb_emit_escaped_c_string(g, str_val);
				}
			} else {
				CB_FAIL(g, "Plan 9 C backend: non-constant string literal\n");
			}
		} else if (expr->tav.value.kind == ExactValue_Integer) {
			String s = big_int_to_string(temporary_allocator(), &expr->tav.value.value_integer);
			gb_fprintf(g->f, "%.*s", LIT(s));
		} else if (expr->tav.value.kind == ExactValue_Float) {
			gb_fprintf(g->f, "%g", expr->tav.value.value_float);
		} else if (expr->tav.value.kind == ExactValue_Bool) {
			gb_fprintf(g->f, "%s", expr->tav.value.value_bool ? "1" : "0");
		} else {
			CB_FAIL(g, "Plan 9 C backend: unsupported literal\n");
		}
	case_end;

	case_ast_node(id, Ident, expr);
		if (expr->tav.mode == Addressing_Value && expr->tav.value.kind != ExactValue_Invalid) {
			cb_emit_exact_value(g, expr->tav.value);
		} else {
			gb_fprintf(g->f, "%.*s", LIT(id->token.string));
		}
	case_end;

	case_ast_node(be, BinaryExpr, expr);
		gb_fprintf(g->f, "(");
		cb_emit_expr(g, be->left);
		gb_fprintf(g->f, " %.*s ", LIT(be->op.string));
		cb_emit_expr(g, be->right);
		gb_fprintf(g->f, ")");
	case_end;

	case_ast_node(de, DerefExpr, expr);
		gb_fprintf(g->f, "(*");
		cb_emit_expr(g, de->expr);
		gb_fprintf(g->f, ")");
	case_end;

	case_ast_node(se, SliceExpr, expr);
		{
			Type *slice_type = type_of_expr(expr);
			Type *src_type = type_of_expr(se->expr);
			Type *base = base_type(type_deref(src_type));
			cb_note_type_for_decl(g, slice_type);
			String slice_name = cb_type_c_name(g, slice_type);
			gbString elem_c = gb_string_make(temporary_allocator(), "");
			if (!cb_type_to_c(g, base_type(slice_type->Slice.elem), &elem_c)) {
				return;
			}

			gb_fprintf(g->f, "((%.*s){", LIT(slice_name));
			if (is_type_pointer(src_type) && base != nullptr && base->kind == Type_Array) {
				gb_fprintf(g->f, "(%s *)", elem_c);
				cb_emit_expr(g, se->expr);
				if (se->low != nullptr) {
					gb_fprintf(g->f, " + ");
					cb_emit_expr(g, se->low);
				}
			} else if (base != nullptr && base->kind == Type_Array) {
				gb_fprintf(g->f, "(%s *)&", elem_c);
				cb_emit_expr(g, se->expr);
				gb_fprintf(g->f, "[");
				if (se->low != nullptr) {
					cb_emit_expr(g, se->low);
				} else {
					gb_fprintf(g->f, "0");
				}
				gb_fprintf(g->f, "]");
			} else if (base != nullptr && base->kind == Type_Slice) {
				gb_fprintf(g->f, "(%s *)", elem_c);
				cb_emit_expr(g, se->expr);
				gb_fprintf(g->f, ".data");
				if (se->low != nullptr) {
					gb_fprintf(g->f, " + ");
					cb_emit_expr(g, se->low);
				}
			} else {
				CB_FAIL(g, "Plan 9 C backend: unsupported slice expression\n");
				return;
			}

			gb_fprintf(g->f, ", ");
			if (se->low != nullptr && se->high != nullptr) {
				gb_fprintf(g->f, "(");
				cb_emit_expr(g, se->high);
				gb_fprintf(g->f, ") - (");
				cb_emit_expr(g, se->low);
				gb_fprintf(g->f, ")");
			} else if (se->low != nullptr && base != nullptr && base->kind == Type_Array) {
				gb_fprintf(g->f, "%lld - (", cast(long long)base->Array.count);
				cb_emit_expr(g, se->low);
				gb_fprintf(g->f, ")");
			} else if (se->high != nullptr) {
				cb_emit_expr(g, se->high);
			} else if (base != nullptr && base->kind == Type_Array) {
				gb_fprintf(g->f, "%lld", cast(long long)base->Array.count);
			} else {
				cb_emit_expr(g, se->expr);
				gb_fprintf(g->f, ".len");
			}

			gb_fprintf(g->f, ", ");
			if (se->low != nullptr && se->high != nullptr) {
				gb_fprintf(g->f, "(");
				cb_emit_expr(g, se->high);
				gb_fprintf(g->f, ") - (");
				cb_emit_expr(g, se->low);
				gb_fprintf(g->f, ")");
			} else if (se->low != nullptr && base != nullptr && base->kind == Type_Array) {
				gb_fprintf(g->f, "%lld - (", cast(long long)base->Array.count);
				cb_emit_expr(g, se->low);
				gb_fprintf(g->f, ")");
			} else if (se->high != nullptr) {
				cb_emit_expr(g, se->high);
			} else if (base != nullptr && base->kind == Type_Array) {
				gb_fprintf(g->f, "%lld", cast(long long)base->Array.count);
			} else {
				cb_emit_expr(g, se->expr);
				gb_fprintf(g->f, ".len");
			}
			gb_fprintf(g->f, "})");
		}
	case_end;

	case_ast_node(ue, UnaryExpr, expr);
		switch (ue->op.kind) {
		case Token_Sub:
		case Token_Not:
		case Token_Xor:
			gb_fprintf(g->f, "%.*s", LIT(ue->op.string));
			cb_emit_expr(g, ue->expr);
			break;
		case Token_And:
			{
				Type *result = default_type(type_of_expr(expr));
				if (is_type_pointer(result)) {
					Type *elem = base_type(type_deref(result));
					if (elem != nullptr && elem->kind == Type_Array) {
						/* Plan 9 C: &array is pointer-to-array; decay for elem* params. */
						cb_emit_expr(g, ue->expr);
						break;
					}
				}
				gb_fprintf(g->f, "&");
				cb_emit_expr(g, ue->expr);
			}
			break;
		case Token_Mul:
			gb_fprintf(g->f, "*");
			cb_emit_expr(g, ue->expr);
			break;
		default:
			CB_FAIL(g, "Plan 9 C backend: unsupported unary operator\n");
			break;
		}
	case_end;

	case_ast_node(se, SelectorExpr, expr);
		cb_emit_expr(g, se->expr);
		if (se->selector != nullptr && se->selector->kind == Ast_Ident) {
			gb_fprintf(g->f, ".%.*s", LIT(se->selector->Ident.token.string));
		} else {
			CB_FAIL(g, "Plan 9 C backend: unsupported selector\n");
		}
	case_end;

	case_ast_node(ie, IndexExpr, expr);
		{
			Type *expr_type = type_of_expr(ie->expr);
			Type *deref = type_deref(expr_type);
			Type *base = base_type(deref);
			cb_emit_expr(g, ie->expr);
			if (is_type_pointer(expr_type) && base != nullptr && base->kind == Type_Array) {
				gb_fprintf(g->f, "[");
			} else if (base != nullptr && base->kind == Type_Slice) {
				gb_fprintf(g->f, ".data[");
			} else {
				gb_fprintf(g->f, "[");
			}
			cb_emit_expr(g, ie->index);
			gb_fprintf(g->f, "]");
		}
	case_end;

	case_ast_node(cl, CompoundLit, expr);
		{
			Type *lit_type = type_of_expr(cl->type);
			cb_emit_type_defs_for_type(g, lit_type);
			String type_name = cb_type_c_name(g, lit_type);
			gb_fprintf(g->f, "((%.*s){", LIT(type_name));
			bool first = true;
			for_array(j, cl->elems) {
				Ast *elem = cl->elems[j];
				if (elem->kind == Ast_FieldValue) {
					if (!first) {
						gb_fprintf(g->f, ", ");
					}
					first = false;
					if (elem->FieldValue.field != nullptr && elem->FieldValue.field->kind == Ast_Ident) {
						gb_fprintf(g->f, ".%.*s = ", LIT(elem->FieldValue.field->Ident.token.string));
					}
					cb_emit_expr(g, elem->FieldValue.value);
				} else {
					if (!first) {
						gb_fprintf(g->f, ", ");
					}
					first = false;
					cb_emit_expr(g, elem);
				}
			}
			gb_fprintf(g->f, "})");
		}
	case_end;

	case_ast_node(ce, CallExpr, expr);
		cb_emit_call_expr(g, expr);
	case_end;

	case_ast_node(tc, TypeCast, expr);
		gbString type_c = gb_string_make(temporary_allocator(), "");
		Type *cast_type = type_of_expr(tc->type);
		if (!cb_type_to_c(g, cast_type, &type_c)) {
			return;
		}
		gb_fprintf(g->f, "(%s)", type_c);
		cb_emit_expr(g, tc->expr);
	case_end;

	default:
		CB_FAIL(g, "Plan 9 C backend: unsupported expression %.*s\n", LIT(ast_strings[expr->kind]));
		break;
	}
}

gb_internal void cb_emit_assign_lhs(cbGen *g, Ast *lhs) {
	lhs = unparen_expr(lhs);
	switch (lhs->kind) {
	case Ast_Ident:
		gb_fprintf(g->f, "%.*s", LIT(lhs->Ident.token.string));
		return;
	case Ast_SelectorExpr:
		cb_emit_expr(g, lhs);
		return;
	case Ast_IndexExpr:
		cb_emit_expr(g, lhs);
		return;
	default:
		CB_FAIL(g, "Plan 9 C backend: unsupported assignment target\n");
		break;
	}
}

gb_internal void cb_emit_value_decl_decls(cbGen *g, Ast *stmt, int indent) {
	ast_node(vd, ValueDecl, stmt);
	for (isize i = 0; i < vd->names.count; i++) {
		Ast *name = vd->names[i];
		if (name->kind != Ast_Ident) {
			continue;
		}
		Entity *e = entity_of_node(name);
		Type *type = e != nullptr ? e->type : nullptr;
		if (type == nullptr && vd->type != nullptr) {
			type = type_of_expr(vd->type);
		}
		if (type == nullptr && vd->values.count > i && vd->values[i] != nullptr) {
			type = type_of_expr(vd->values[i]);
		}
		if (type == nullptr) {
			CB_FAIL(g, "Plan 9 C backend: could not determine type for %.*s\n", LIT(name->Ident.token.string));
			return;
		}

		cb_indent(g, indent);
		cb_emit_c_var(g, type, name->Ident.token.string);
		gb_fprintf(g->f, ";\n");
	}
}

gb_internal void cb_emit_value_decl_inits(cbGen *g, Ast *stmt, int indent) {
	ast_node(vd, ValueDecl, stmt);
	for (isize i = 0; i < vd->names.count; i++) {
		if (i >= vd->values.count || vd->values[i] == nullptr) {
			continue;
		}
		Ast *name = vd->names[i];
		if (name->kind != Ast_Ident) {
			continue;
		}
		if (vd->values[i]->kind == Ast_CompoundLit) {
			ast_node(cl, CompoundLit, vd->values[i]);
			for_array(j, cl->elems) {
				Ast *elem = cl->elems[j];
				if (elem->kind != Ast_FieldValue) {
					continue;
				}
				if (elem->FieldValue.field == nullptr || elem->FieldValue.field->kind != Ast_Ident) {
					continue;
				}
				cb_indent(g, indent);
				gb_fprintf(g->f, "%.*s.%.*s = ", LIT(name->Ident.token.string), LIT(elem->FieldValue.field->Ident.token.string));
				cb_emit_expr(g, elem->FieldValue.value);
				gb_fprintf(g->f, ";\n");
			}
			continue;
		}
		cb_indent(g, indent);
		gb_fprintf(g->f, "%.*s = ", LIT(name->Ident.token.string));
		cb_emit_expr(g, vd->values[i]);
		gb_fprintf(g->f, ";\n");
	}
}

gb_internal void cb_hoist_value_decl(cbGen *g, Ast *init, int indent) {
	if (init != nullptr && init->kind == Ast_ValueDecl) {
		cb_emit_value_decl_decls(g, init, indent);
	}
}

gb_internal void cb_hoist_block_decls(cbGen *g, Ast *block, int indent) {
	if (block == nullptr || block->kind != Ast_BlockStmt) {
		return;
	}
	ast_node(bs, BlockStmt, block);

	for (Ast *stmt : bs->stmts) {
		switch (stmt->kind) {
		case Ast_ValueDecl:
			cb_emit_value_decl_decls(g, stmt, indent);
			break;
		case Ast_IfStmt:
			cb_hoist_value_decl(g, stmt->IfStmt.init, indent);
			break;
		case Ast_ForStmt:
			cb_hoist_value_decl(g, stmt->ForStmt.init, indent);
			break;
		case Ast_RangeStmt:
			if (stmt->RangeStmt.init != nullptr && stmt->RangeStmt.init->kind == Ast_ValueDecl) {
				cb_emit_value_decl_decls(g, stmt->RangeStmt.init, indent);
			}
			if (stmt->RangeStmt.vals.count > 1) {
				Ast *val1 = stmt->RangeStmt.vals[1];
				if (val1 != nullptr && !is_blank_ident(val1) && val1->kind == Ast_Ident) {
					Type *index_type = type_of_expr(val1);
					if (index_type == nullptr) {
						index_type = t_int;
					}
					cb_indent(g, indent);
					cb_emit_c_var(g, index_type, val1->Ident.token.string);
					gb_fprintf(g->f, ";\n");
				}
			}
			if (stmt->RangeStmt.vals.count > 0) {
				Ast *val0 = stmt->RangeStmt.vals[0];
				if (val0 != nullptr && !is_blank_ident(val0) && val0->kind == Ast_Ident) {
					Type *val_type = type_of_expr(val0);
					if (val_type != nullptr) {
						cb_indent(g, indent);
						cb_emit_c_var(g, val_type, val0->Ident.token.string);
						gb_fprintf(g->f, ";\n");
					}
				}
			}
			break;
		}
	}
}

gb_internal void cb_emit_init_decls(cbGen *g, Ast *init, int indent) {
	/* Declarations are hoisted to block start for C89 (Plan 9). */
	(void)g; (void)init; (void)indent;
}

gb_internal void cb_emit_init_expr(cbGen *g, Ast *init, int indent) {
	if (init == nullptr) {
		return;
	}
	switch (init->kind) {
	case Ast_ValueDecl:
		cb_emit_value_decl_inits(g, init, indent);
		break;
	case Ast_AssignStmt:
		cb_indent(g, indent);
		cb_emit_assign_lhs(g, init->AssignStmt.lhs[0]);
		if (init->AssignStmt.op.kind == Token_Eq) {
			gb_fprintf(g->f, " = ");
			cb_emit_expr(g, init->AssignStmt.rhs[0]);
		} else {
			String op = {};
			if (!cb_token_to_c_op(init->AssignStmt.op.kind, &op)) {
				CB_FAIL(g, "Plan 9 C backend: unsupported assign operator\n");
				return;
			}
			gb_fprintf(g->f, " %.*s ", LIT(op));
			cb_emit_expr(g, init->AssignStmt.rhs[0]);
		}
		gb_fprintf(g->f, ";\n");
		break;
	case Ast_ExprStmt:
		cb_indent(g, indent);
		cb_emit_expr(g, init->ExprStmt.expr);
		gb_fprintf(g->f, ";\n");
		break;
	default:
		CB_FAIL(g, "Plan 9 C backend: unsupported init statement %.*s\n", LIT(ast_strings[init->kind]));
		break;
	}
}

gb_internal void cb_emit_for_init_expr_only(cbGen *g, Ast *init) {
	if (init == nullptr) {
		return;
	}
	switch (init->kind) {
	case Ast_ValueDecl:
		if (init->ValueDecl.values.count > 0 && init->ValueDecl.values[0] != nullptr) {
			Ast *name = init->ValueDecl.names[0];
			if (name->kind == Ast_Ident) {
				gb_fprintf(g->f, "%.*s = ", LIT(name->Ident.token.string));
				cb_emit_expr(g, init->ValueDecl.values[0]);
			}
		}
		break;
	case Ast_AssignStmt:
		cb_emit_assign_lhs(g, init->AssignStmt.lhs[0]);
		if (init->AssignStmt.op.kind == Token_Eq) {
			gb_fprintf(g->f, " = ");
			cb_emit_expr(g, init->AssignStmt.rhs[0]);
		} else {
			String op = {};
			if (!cb_token_to_c_op(init->AssignStmt.op.kind, &op)) {
				CB_FAIL(g, "Plan 9 C backend: unsupported for-init operator\n");
				return;
			}
			gb_fprintf(g->f, " %.*s ", LIT(op));
			cb_emit_expr(g, init->AssignStmt.rhs[0]);
		}
		break;
	case Ast_ExprStmt:
		cb_emit_expr(g, init->ExprStmt.expr);
		break;
	default:
		CB_FAIL(g, "Plan 9 C backend: unsupported for-init %.*s\n", LIT(ast_strings[init->kind]));
		break;
	}
}

gb_internal void cb_emit_else(cbGen *g, Ast *else_stmt, int indent) {
	if (else_stmt == nullptr) {
		return;
	}
	if (else_stmt->kind == Ast_IfStmt && else_stmt->IfStmt.init == nullptr) {
		ast_node(is, IfStmt, else_stmt);
		cb_indent(g, indent);
		gb_fprintf(g->f, "else if (");
		cb_emit_expr(g, is->cond);
		gb_fprintf(g->f, ") {\n");
		if (is->body->kind == Ast_BlockStmt) {
			cb_emit_block(g, is->body, indent+1);
		} else {
			cb_emit_stmt(g, is->body, indent+1);
		}
		cb_indent(g, indent);
		gb_fprintf(g->f, "}\n");
		cb_emit_else(g, is->else_stmt, indent);
		return;
	}
	cb_indent(g, indent);
	gb_fprintf(g->f, "else {\n");
	if (else_stmt->kind == Ast_BlockStmt) {
		cb_emit_block(g, else_stmt, indent+1);
	} else {
		cb_emit_stmt(g, else_stmt, indent+1);
	}
	cb_indent(g, indent);
	gb_fprintf(g->f, "}\n");
}

gb_internal void cb_emit_if_stmt(cbGen *g, Ast *stmt, int indent) {
	ast_node(is, IfStmt, stmt);

	cb_emit_init_expr(g, is->init, indent);

	cb_indent(g, indent);
	gb_fprintf(g->f, "if (");
	cb_emit_expr(g, is->cond);
	gb_fprintf(g->f, ") {\n");

	if (is->body->kind == Ast_BlockStmt) {
		cb_emit_block(g, is->body, indent+1);
	} else {
		cb_emit_stmt(g, is->body, indent+1);
	}

	cb_indent(g, indent);
	gb_fprintf(g->f, "}\n");
	cb_emit_else(g, is->else_stmt, indent);
}

gb_internal void cb_emit_range_stmt(cbGen *g, Ast *stmt, int indent) {
	ast_node(rs, RangeStmt, stmt);

	Type *expr_type = base_type(type_deref(type_of_expr(rs->expr)));
	if (expr_type == nullptr || expr_type->kind != Type_Slice) {
		CB_FAIL(g, "Plan 9 C backend: range only supported for slices\n");
		return;
	}

	cb_emit_init_decls(g, rs->init, indent);
	cb_emit_init_expr(g, rs->init, indent);

	Ast *val0 = rs->vals.count > 0 ? rs->vals[0] : nullptr;
	Ast *val1 = rs->vals.count > 1 ? rs->vals[1] : nullptr;

	String index_name = str_lit("i");
	if (val1 != nullptr && !is_blank_ident(val1) && val1->kind == Ast_Ident) {
		index_name = val1->Ident.token.string;
	}

	cb_indent(g, indent);
	gb_fprintf(g->f, "for (%.*s = 0; ", LIT(index_name));
	cb_emit_expr(g, rs->expr);
	gb_fprintf(g->f, ".len > %.*s; %.*s += 1) {\n", LIT(index_name), LIT(index_name));

	if (val0 != nullptr && !is_blank_ident(val0) && val0->kind == Ast_Ident) {
		cb_indent(g, indent+1);
		cb_emit_assign_lhs(g, val0);
		gb_fprintf(g->f, " = ");
		cb_emit_expr(g, rs->expr);
		gb_fprintf(g->f, ".data[%.*s];\n", LIT(index_name));
	}

	if (rs->body->kind == Ast_BlockStmt) {
		cb_emit_block(g, rs->body, indent+1);
	} else {
		cb_emit_stmt(g, rs->body, indent+1);
	}

	cb_indent(g, indent);
	gb_fprintf(g->f, "}\n");
}

gb_internal void cb_emit_for_stmt(cbGen *g, Ast *stmt, int indent) {
	ast_node(fs, ForStmt, stmt);

	cb_indent(g, indent);
	gb_fprintf(g->f, "for (");
	cb_emit_for_init_expr_only(g, fs->init);
	gb_fprintf(g->f, "; ");
	if (fs->cond != nullptr) {
		cb_emit_expr(g, fs->cond);
	}
	gb_fprintf(g->f, "; ");
	if (fs->post != nullptr) {
		switch (fs->post->kind) {
		case Ast_AssignStmt:
			cb_emit_assign_lhs(g, fs->post->AssignStmt.lhs[0]);
			if (fs->post->AssignStmt.op.kind == Token_Eq) {
				gb_fprintf(g->f, " = ");
				cb_emit_expr(g, fs->post->AssignStmt.rhs[0]);
			} else {
				String op = {};
				if (!cb_token_to_c_op(fs->post->AssignStmt.op.kind, &op)) {
					CB_FAIL(g, "Plan 9 C backend: unsupported for-post operator\n");
					return;
				}
				gb_fprintf(g->f, " %.*s ", LIT(op));
				cb_emit_expr(g, fs->post->AssignStmt.rhs[0]);
			}
			break;
		case Ast_ExprStmt:
			cb_emit_expr(g, fs->post->ExprStmt.expr);
			break;
		default:
			CB_FAIL(g, "Plan 9 C backend: unsupported for-post %.*s\n", LIT(ast_strings[fs->post->kind]));
			return;
		}
	}
	gb_fprintf(g->f, ") {\n");

	if (fs->body->kind == Ast_BlockStmt) {
		cb_emit_block(g, fs->body, indent+1);
	} else {
		cb_emit_stmt(g, fs->body, indent+1);
	}

	cb_indent(g, indent);
	gb_fprintf(g->f, "}\n");
}

gb_internal void cb_emit_switch_stmt(cbGen *g, Ast *stmt, int indent) {
	ast_node(ss, SwitchStmt, stmt);

	cb_emit_init_decls(g, ss->init, indent);
	cb_emit_init_expr(g, ss->init, indent);

	cb_indent(g, indent);
	gb_fprintf(g->f, "switch (");
	if (ss->tag != nullptr) {
		cb_emit_expr(g, ss->tag);
	} else {
		gb_fprintf(g->f, "0");
	}
	gb_fprintf(g->f, ") {\n");

	if (ss->body != nullptr && ss->body->kind == Ast_BlockStmt) {
		ast_node(bs, BlockStmt, ss->body);
		for (Ast *clause : bs->stmts) {
			if (clause->kind != Ast_CaseClause) {
				continue;
			}
			ast_node(cc, CaseClause, clause);
			if (cc->list.count == 0) {
				cb_indent(g, indent+1);
				gb_fprintf(g->f, "default:\n");
			} else {
				for (isize i = 0; i < cc->list.count; i++) {
					cb_indent(g, indent+1);
					gb_fprintf(g->f, "case ");
					cb_emit_expr(g, cc->list[i]);
					gb_fprintf(g->f, ":\n");
				}
			}
			for (Ast *s : cc->stmts) {
				cb_emit_stmt(g, s, indent+2);
			}
			cb_indent(g, indent+1);
			gb_fprintf(g->f, "break;\n");
		}
	}

	cb_indent(g, indent);
	gb_fprintf(g->f, "}\n");
}

gb_internal void cb_emit_stmt(cbGen *g, Ast *stmt, int indent) {
	if (g->failed || stmt == nullptr) {
		return;
	}

	switch (stmt->kind) {
	case Ast_EmptyStmt:
		break;

	case Ast_ValueDecl:
		cb_emit_value_decl_decls(g, stmt, indent);
		cb_emit_value_decl_inits(g, stmt, indent);
		break;

	case Ast_AssignStmt:
		cb_indent(g, indent);
		cb_emit_assign_lhs(g, stmt->AssignStmt.lhs[0]);
		if (stmt->AssignStmt.op.kind == Token_Eq) {
			gb_fprintf(g->f, " = ");
			cb_emit_expr(g, stmt->AssignStmt.rhs[0]);
		} else {
			String op = {};
			if (!cb_token_to_c_op(stmt->AssignStmt.op.kind, &op)) {
				CB_FAIL(g, "Plan 9 C backend: unsupported assign operator\n");
				return;
			}
			gb_fprintf(g->f, " %.*s ", LIT(op));
			cb_emit_expr(g, stmt->AssignStmt.rhs[0]);
		}
		gb_fprintf(g->f, ";\n");
		break;

	case Ast_ExprStmt:
		if (stmt->ExprStmt.expr != nullptr &&
		    stmt->ExprStmt.expr->kind == Ast_CallExpr &&
		    cb_call_needs_variadic_pack(stmt->ExprStmt.expr)) {
			cb_emit_variadic_call_stmts(g, stmt->ExprStmt.expr, indent);
		} else {
			cb_indent(g, indent);
			cb_emit_expr(g, stmt->ExprStmt.expr);
			gb_fprintf(g->f, ";\n");
		}
		break;

	case Ast_ReturnStmt:
		g->has_return = true;
		cb_indent(g, indent);
		if (stmt->ReturnStmt.results.count == 0) {
			if (g->proc_result_count == 0) {
				gb_fprintf(g->f, "return;\n");
			} else {
				gb_fprintf(g->f, "return 0;\n");
			}
		} else if (stmt->ReturnStmt.results.count == 1) {
			Ast *result = stmt->ReturnStmt.results[0];
			if (result->kind == Ast_CallExpr && cb_call_needs_variadic_pack(result)) {
				CB_FAIL(g, "Plan 9 C backend: variadic call return not supported yet\n");
			} else {
				gb_fprintf(g->f, "return ");
				cb_emit_expr(g, result);
				gb_fprintf(g->f, ";\n");
			}
		} else {
			CB_FAIL(g, "Plan 9 C backend: multi-value return not supported yet\n");
		}
		break;

	case Ast_BlockStmt:
		cb_emit_block(g, stmt, indent);
		break;

	case Ast_IfStmt:
		cb_emit_if_stmt(g, stmt, indent);
		break;

	case Ast_ForStmt:
		cb_emit_for_stmt(g, stmt, indent);
		break;

	case Ast_RangeStmt:
		cb_emit_range_stmt(g, stmt, indent);
		break;

	case Ast_SwitchStmt:
		cb_emit_switch_stmt(g, stmt, indent);
		break;

	default:
		CB_FAIL(g, "Plan 9 C backend: unsupported statement %.*s\n", LIT(ast_strings[stmt->kind]));
		break;
	}
}

gb_internal void cb_emit_block(cbGen *g, Ast *block, int indent) {
	if (block == nullptr) {
		return;
	}
	ast_node(bs, BlockStmt, block);

	cb_hoist_block_decls(g, block, indent);

	for (Ast *stmt : bs->stmts) {
		if (stmt->kind == Ast_ValueDecl) {
			cb_emit_value_decl_inits(g, stmt, indent);
		} else {
			cb_emit_stmt(g, stmt, indent);
		}
		if (g->failed) {
			return;
		}
	}
}

gb_internal bool cb_should_emit_proc(cbGen *g, Entity *entity, bool allow_entry_point) {
	if (entity == nullptr || entity->kind != Entity_Procedure) {
		return false;
	}
	if (entity->Procedure.is_foreign) {
		return false;
	}
	if (!allow_entry_point && entity == g->checker->info.entry_point) {
		return false;
	}
	if (cb_should_skip_compiler_proc(entity)) {
		return false;
	}
	DeclInfo *decl = decl_info_of_entity(entity);
	if (decl == nullptr || decl->proc_lit == nullptr) {
		return false;
	}
	Type *type = base_type(entity->type);
	if (type == nullptr || type->kind != Type_Proc) {
		return false;
	}
	if (entity->min_dep_count.load(std::memory_order_relaxed) == 0) {
		return false;
	}
	if (!cb_is_emittable_package(entity->pkg, &g->checker->info)) {
		return false;
	}
	return true;
}

gb_internal void cb_emit_proc_fwd_decl(cbGen *g, Entity *entity, bool allow_entry_point) {
	if (!cb_should_emit_proc(g, entity, allow_entry_point)) {
		return;
	}

	Type *type = base_type(entity->type);
	TypeProc *pt = &type->Proc;

	gbString ret_c = gb_string_make(temporary_allocator(), "void");
	if (pt->result_count > 0 && pt->results != nullptr) {
		ret_c = gb_string_make(temporary_allocator(), "");
		if (!cb_type_to_c(g, pt->results->Tuple.variables[0]->type, &ret_c)) {
			return;
		}
	}

	if (allow_entry_point && entity == g->checker->info.entry_point) {
		gb_fprintf(g->f, "static %s %s(", ret_c, CB_ODIN_USER_MAIN_NAME);
	} else {
		gb_fprintf(g->f, "%s %.*s(", ret_c, LIT(entity->token.string));
	}
	if (pt->param_count > 0 && pt->params != nullptr) {
		for (isize i = 0; i < pt->param_count; i++) {
			if (i > 0) {
				gb_fprintf(g->f, ", ");
			}
			Entity *param = pt->params->Tuple.variables[i];
			cb_emit_c_var(g, param->type, param->token.string);
		}
	} else {
		gb_fprintf(g->f, "void");
	}
	gb_fprintf(g->f, ");\n");
}

gb_internal void cb_emit_package_proc_fwd_decls(cbGen *g, Checker *checker, bool allow_entry_point) {
	CheckerInfo *info = &checker->info;
	for (isize i = 0; i < info->entities.count; i++) {
		cb_emit_proc_fwd_decl(g, info->entities[i], allow_entry_point);
		if (g->failed) {
			return;
		}
	}
	gb_fprintf(g->f, "\n");
}

gb_internal void cb_emit_proc(cbGen *g, Entity *entity, bool allow_entry_point) {
	if (!cb_should_emit_proc(g, entity, allow_entry_point)) {
		return;
	}

	DeclInfo *decl = decl_info_of_entity(entity);
	Type *type = base_type(entity->type);
	TypeProc *pt = &type->Proc;

	gbString ret_c = gb_string_make(temporary_allocator(), "void");
	if (pt->result_count > 0 && pt->results != nullptr) {
		ret_c = gb_string_make(temporary_allocator(), "");
		if (!cb_type_to_c(g, pt->results->Tuple.variables[0]->type, &ret_c)) {
			return;
		}
	}

	if (allow_entry_point && entity == g->checker->info.entry_point) {
		gb_fprintf(g->f, "static %s\n", ret_c);
	} else {
		gb_fprintf(g->f, "%s\n", ret_c);
	}
	if (allow_entry_point && entity == g->checker->info.entry_point) {
		gb_fprintf(g->f, "%s(", CB_ODIN_USER_MAIN_NAME);
	} else {
		gb_fprintf(g->f, "%.*s(", LIT(entity->token.string));
	}
	if (pt->param_count > 0 && pt->params != nullptr) {
		for (isize i = 0; i < pt->param_count; i++) {
			if (i > 0) {
				gb_fprintf(g->f, ", ");
			}
			Entity *param = pt->params->Tuple.variables[i];
			cb_emit_c_var(g, param->type, param->token.string);
		}
	} else {
		gb_fprintf(g->f, "void");
	}
	gb_fprintf(g->f, ")\n");
	gb_fprintf(g->f, "{\n");

	bool saved_has_return = g->has_return;
	i32 saved_proc_result_count = g->proc_result_count;
	g->has_return = false;
	g->proc_result_count = pt->result_count;

	ast_node(pl, ProcLit, decl->proc_lit);
	cb_emit_block(g, pl->body, 1);

	if (!g->failed && !g->has_return) {
		if (pt->result_count == 0) {
			gb_fprintf(g->f, "\treturn;\n");
		} else {
			gb_fprintf(g->f, "\treturn 0;\n");
		}
	}
	gb_fprintf(g->f, "}\n\n");

	g->has_return = saved_has_return;
	g->proc_result_count = saved_proc_result_count;
}

gb_internal void cb_collect_scope_foreign_procs(cbGen *g, Scope *scope) {
	if (scope == nullptr) {
		return;
	}
	for (auto const &entry : scope->elements) {
		cb_note_foreign_proc(g, entry.value);
	}
}

gb_internal void cb_emit_foreign_proc_decl(cbGen *g, Entity *e) {
	Type *type = base_type(e->type);
	if (type == nullptr || type->kind != Type_Proc) {
		return;
	}

	TypeProc *pt = &type->Proc;
	gbString ret_c = gb_string_make(temporary_allocator(), "void");
	if (pt->result_count > 0 && pt->results != nullptr) {
		Type *result = pt->results->Tuple.variables[0]->type;
		ret_c = gb_string_make(temporary_allocator(), "");
		if (!cb_type_to_c(g, result, &ret_c)) {
			return;
		}
	}

	gb_fprintf(g->f, "extern %s %.*s(", ret_c, LIT(cb_entity_link_name(e)));
	if (pt->params != nullptr) {
		for (isize i = 0; i < pt->param_count; i++) {
			Entity *param = pt->params->Tuple.variables[i];
			if (i > 0) {
				gb_fprintf(g->f, ", ");
			}
			cb_emit_c_var(g, param->type, param->token.string);
		}
	}
	gb_fprintf(g->f, ");\n");
}

gb_internal void cb_emit_foreign_decls(cbGen *g) {
	Entity *e = nullptr;
	FOR_PTR_SET(e, g->foreign_procs) {
	 String name = cb_entity_link_name(e);
		if (cb_is_runtime_foreign(name)) {
			continue;
		}
		cb_emit_foreign_proc_decl(g, e);
	}
}

gb_internal void cb_precollect_proc_types(cbGen *g, Entity *entity) {
	if (entity == nullptr || entity->kind != Entity_Procedure || entity->Procedure.is_foreign) {
		return;
	}

	Type *type = entity->type;
	if (type == nullptr) {
		return;
	}
	type = base_type(type);
	if (type == nullptr || type->kind != Type_Proc) {
		return;
	}
	TypeProc *pt = &type->Proc;

	if (pt->params != nullptr) {
		for (isize i = 0; i < pt->param_count; i++) {
			cb_emit_type_defs_for_type(g, pt->params->Tuple.variables[i]->type);
		}
	}
	if (pt->results != nullptr) {
		for (isize i = 0; i < pt->result_count; i++) {
			cb_emit_type_defs_for_type(g, pt->results->Tuple.variables[i]->type);
		}
	}
}

gb_internal void cb_emit_all_type_defs(cbGen *g, Checker *checker) {
	CheckerInfo *info = &checker->info;

	for (isize i = 0; i < info->entities.count; i++) {
		Entity *e = info->entities[i];
		if (e->pkg != info->init_package) {
			continue;
		}
		if (e->kind == Entity_TypeName && e->type != nullptr) {
			String name = cb_sanitize_c_name(e->token.string);
			Type *base = base_type(e->type);
			if (base != nullptr) {
				map_set(&g->emitted_type_names, base, name);
				map_set(&g->emitted_type_names, e->type, name);
			}
			cb_emit_type_defs_for_type(g, e->type);
			if (g->failed) {
				return;
			}
		}
	}

	for (isize i = 0; i < info->entities.count; i++) {
		Entity *e = info->entities[i];
		if (e->kind != Entity_Procedure || e->Procedure.is_foreign) {
			continue;
		}
		if (!cb_is_emittable_package(e->pkg, info)) {
			continue;
		}
		if (e->min_dep_count.load(std::memory_order_relaxed) == 0) {
			continue;
		}
		if (cb_should_skip_compiler_proc(e)) {
			continue;
		}
		cb_precollect_proc_types(g, e);
		if (g->failed) {
			return;
		}
	}
}

gb_internal void cb_emit_package_procs(cbGen *g, Checker *checker, bool allow_entry_point) {
	CheckerInfo *info = &checker->info;

	for (isize i = 0; i < info->entities.count; i++) {
		Entity *e = info->entities[i];
		if (e->kind != Entity_Procedure || e->Procedure.is_foreign) {
			continue;
		}
		if (!cb_is_emittable_package(e->pkg, info)) {
			continue;
		}
		if (e->min_dep_count.load(std::memory_order_relaxed) == 0) {
			continue;
		}
		cb_emit_proc(g, e, allow_entry_point);
		if (g->failed) {
			return;
		}
	}
}

gb_internal void cb_emit_runtime_glue(cbGen *g, Checker *checker) {
	if (!cb_wants_runtime_emission()) {
		return;
	}

	CheckerInfo *info = &checker->info;

	gb_fprintf(g->f, "static void\n");
	gb_fprintf(g->f, "__$startup_runtime(void)\n");
	gb_fprintf(g->f, "{\n");
	for (Entity *e : info->init_procedures) {
		if (e == nullptr || e->kind != Entity_Procedure || e->Procedure.is_foreign) {
			continue;
		}
		if (cb_should_skip_compiler_proc(e)) {
			continue;
		}
		gb_fprintf(g->f, "\t%.*s();\n", LIT(e->token.string));
	}
	gb_fprintf(g->f, "}\n\n");

	gb_fprintf(g->f, "static void\n");
	gb_fprintf(g->f, "__$cleanup_runtime(void)\n");
	gb_fprintf(g->f, "{\n");
	gb_fprintf(g->f, "}\n\n");
}

gb_internal void cb_emit_user_procs(cbGen *g, Checker *checker) {
	cb_emit_package_procs(g, checker, false);
}

gb_internal bool cb_emit_program(cbGen *g, Checker *checker) {
	CheckerInfo *info = &checker->info;
	Entity *entry_point = info->entry_point;

	if (!build_context.no_entry_point && entry_point == nullptr) {
		gb_printf_err("No `main` procedure found for Plan 9 C output\n");
		return false;
	}

	cb_collect_scope_foreign_procs(g, info->init_scope);

	gb_fprintf(g->f, "/* Generated by odin -backend:plan9-c — do not edit. */\n");
	if (entry_point != nullptr) {
		String pkg = str_lit("");
		if (entry_point->pkg != nullptr) {
			pkg = entry_point->pkg->name;
		}
		gb_fprintf(g->f, "/* entry point: %.*s", LIT(entry_point->token.string));
		if (pkg.len > 0) {
			gb_fprintf(g->f, " (package %.*s)", LIT(pkg));
		}
		gb_fprintf(g->f, " */\n");
	}
	gb_fprintf(g->f, "#include \"odin_generated.h\"\n\n");

	cb_emit_foreign_decls(g);
	cb_emit_all_type_defs(g, checker);
	cb_collect_types_from_checker(g, checker);
	cb_emit_pending_type_defs(g);
	cb_emit_runtime_glue(g, checker);
	cb_emit_package_proc_fwd_decls(g, checker, true);
	cb_emit_package_procs(g, checker, true);

	gb_fprintf(g->f, "int\n");
	gb_fprintf(g->f, "odin_main(int argc, char **argv)\n");
	gb_fprintf(g->f, "{\n");
	gb_fprintf(g->f, "\tUSED(argc);\n");
	gb_fprintf(g->f, "\tUSED(argv);\n");

	if (cb_wants_runtime_emission()) {
		gb_fprintf(g->f, "\t__$startup_runtime();\n");
	}

	if (entry_point != nullptr) {
		DeclInfo *decl = decl_info_of_entity(entry_point);
		if (decl != nullptr && decl->proc_lit != nullptr) {
			Type *type = base_type(entry_point->type);
			if (type != nullptr && type->kind == Type_Proc) {
				TypeProc *pt = &type->Proc;
				if (pt->param_count == 0) {
					gb_fprintf(g->f, "\t%s();\n", CB_ODIN_USER_MAIN_NAME);
				} else {
					/* Inline entry body when it takes parameters (unusual). */
					ast_node(pl, ProcLit, decl->proc_lit);
					cb_emit_block(g, pl->body, 1);
				}
			}
		}
	}

	if (cb_wants_runtime_emission()) {
		gb_fprintf(g->f, "\t__$cleanup_runtime();\n");
	}

	if (!g->failed && !g->has_return) {
		gb_fprintf(g->f, "\treturn 0;\n");
	}
	gb_fprintf(g->f, "}\n");
	return !g->failed;
}

gb_internal bool cb_generate_code(Checker *checker) {
	String output_path = path_to_string(temporary_allocator(), build_context.build_paths[BuildPath_Output]);
	char const *filename = alloc_cstring(temporary_allocator(), output_path);

	gbFile f = {};
	gbFileError err = gb_file_create(&f, filename);
	if (err != gbFileError_None) {
		gb_printf_err("Failed to create Plan 9 C output file: %s\n", filename);
		return false;
	}
	defer (gb_file_close(&f));

	cbGen gen = {};
	gen.f = &f;
	gen.checker = checker;
	ptr_set_init(&gen.foreign_procs);
	map_init(&gen.emitted_type_names);
	ptr_set_init(&gen.emitted_type_defs);
	ptr_set_init(&gen.pending_type_defs);

	bool ok = cb_emit_program(&gen, checker);
	if (ok) {
		gb_printf("Plan 9 C output written to %.*s\n", LIT(output_path));
	}
	return ok;
}

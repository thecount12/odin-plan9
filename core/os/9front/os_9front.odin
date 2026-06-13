package os_9front

import sys "string_to_be_replaced_with_relative_path_later"

// Standard handles mapping to 9front's default open FDs
stdin  : Handle = 0;
stdout : Handle = 1;
stderr : Handle = 2;

Handle :: distinct i32;

write :: proc(fd: Handle, data: []byte) -> (int, File_Error) {
    if len(data) == 0 do return 0, .None;
    
    bytes_written := sys.write(i32(fd), raw_data(data), len(data));
    if bytes_written < 0 {
        return 0, .Unknown_Error; // You can refine errors later
    }
    return bytes_written, .None;
}


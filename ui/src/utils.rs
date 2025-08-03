use procfs::{process, ProcResult};
use std::path::Path;

pub fn find_vulkan_processes() -> ProcResult<Vec<(String, String)>> {
    let mut processes = Vec::new();
    let apps = process::all_processes()?;

    for app in apps {
        let Ok(process) = app else { continue; };

        // ensure vulkan is loaded
        let Ok(maps) = proc_maps::get_process_maps(process.pid()) else {
            continue;
        };

        let result = maps.iter()
            .filter_map(|map| map.filename())
            .map(|filename| filename.to_string_lossy().to_string())
            .any(|filename| filename.to_lowercase().contains("vulkan"));

        if !result {
            continue;
        }

        // Format process information

        let pid = process.pid();

        // By default, assume Linux process, and use /proc/self/comm
        let mut name = process.stat()?.comm;

        // Solely just for checking if it's a Proton or Wine process
        // Not sure if there's a more efficient way to do this
        let cmdline = process.cmdline()?.join(" ");
        

        // If this is a Proton or Wine process with .exe,
        // then just get filename from /proc/self/maps
        if cmdline.contains(".exe") {
            for map in &maps {
                if let Some(path) = map.filename() {
                    let path_str = path.to_string_lossy().to_lowercase();
                    if path_str.contains(".exe") &&
                       !path_str.contains("wine") && // Make sure .exe is not from Wine
                       path_str.contains('/') ||
                       path_str.contains('\\') {

                        name = Path::new(&path_str).file_name()
                                                             .unwrap_or_default()
                                                             .to_string_lossy()
                                                             .to_string();
                        break;
                    }
                }
            }
        }
        let process_info = format!("PID {pid}: {name}");
        processes.push((process_info, name));
    }

    Ok(processes)
}

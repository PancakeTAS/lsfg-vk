use procfs::{process, ProcResult};
use regex::Regex;

pub fn find_vulkan_processes() -> ProcResult<Vec<(String, String)>> {

    // Extract just .exe name from a running Wine or Proton process
    let pattern = Regex::new(r"[-\w\s\.()\[\]!@]*(\.[Ee][Xx][Ee])").unwrap();

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

        // format process information
        let pid = process.pid();
        let name: String;
        let cmdline = process.cmdline()?.join(" ");

        // If this is a Proton or Wine process with .exe,
        // then extract just the .exe name with RegEx
        if cmdline.contains(".exe") {
            name = pattern.find(&cmdline)
                          .unwrap()
                          .as_str()
                          .to_string();
        } else {

        // If just a normal Linux process, then use
        // the normal name
            name = process.stat()?.comm;
        }

        let process_info = format!("PID {pid}: {name}");
        processes.push((process_info, name));
    }

    Ok(processes)
}

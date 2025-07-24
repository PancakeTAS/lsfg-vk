use procfs::{process, ProcResult};

pub fn find_vulkan_processes() -> ProcResult<Vec<(String, String)>> {
    let mut processes = Vec::new();
    let apps = process::all_processes()?;
    for app in apps {
        let Ok(prc) = app else { continue; };

        // ensure vulkan is loaded
        let Ok(maps) = proc_maps::get_process_maps(prc.pid()) else {
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
        let pid = prc.pid();
        let name = prc.stat()?.comm;
        let process_info = format!("PID {}: {}", pid, name);
        processes.push((process_info, name));
    }

    Ok(processes)
}

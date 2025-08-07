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

        // find executed binary
        let mut exe = prc.exe()?.to_string_lossy().to_string();

        // replace binary with exe for wine apps
        if exe.contains("wine") || exe.contains("proton") {
            let result = maps.iter()
                .filter_map(|map| map.filename())
                .map(|filename| filename.to_string_lossy().to_string())
                .find(|filename| filename.ends_with(".exe"));

            if let Some(exe_name) = result {
                exe = exe_name;
            }
        }

        // split off last part of the path
        exe = exe.split('/').last().unwrap_or(&exe).to_string();

        // format process information
        let pid = prc.pid();
        let process_info = format!("PID {}: {}", pid, exe);
        processes.push((process_info, exe));
    }

    Ok(processes)
}

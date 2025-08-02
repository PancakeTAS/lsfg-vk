use std::sync::{Arc, OnceLock, RwLock};

use anyhow::Context;

pub mod structs;
pub use structs::*;

/// Find the MangoHUD configuration file path
fn find_mangohud_config_file() -> String {
    if let Some(home) = std::env::var("HOME").ok() {
        return format!("{}/.config/MangoHud/MangoHud.conf", home);
    }
    "MangoHud.conf".to_string()
}

/// Find the configuration file path based on environment variables
fn find_config_file() -> String {
    if let Some(path) = std::env::var("LSFG_CONFIG").ok() {
        return path;
    }

    if let Some(xdg) = std::env::var("XDG_CONFIG_HOME").ok() {
        return format!("{}/lsfg-vk/conf.toml", xdg);
    }

    if let Some(home) = std::env::var("HOME").ok() {
        return format!("{}/.config/lsfg-vk/conf.toml", home);
    }

    "conf.toml".to_string()
}

static CONFIG: OnceLock<Arc<RwLock<TomlConfig>>> = OnceLock::new();
static CONFIG_WRITER: OnceLock<std::sync::mpsc::Sender<()>> = OnceLock::new();

pub fn default_config() -> TomlConfig {
    TomlConfig {
        version: 1,
        global: TomlGlobal {
            dll: None,
            mangohud: TomlMangoHud::default(),
        },
        game: vec![
            TomlGame {
                exe: String::from("vkcube"),
                multiplier: Multiplier::from(4),
                flow_scale: FlowScale::from(0.7),
                performance_mode: true,
                hdr_mode: false,
                experimental_present_mode: PresentMode::Vsync,
            },
            TomlGame {
                exe: String::from("benchmark"),
                multiplier: Multiplier::from(4),
                flow_scale: FlowScale::from(1.0),
                performance_mode: true,
                hdr_mode: false,
                experimental_present_mode: PresentMode::Vsync,
            },
            TomlGame {
                exe: String::from("Genshin"),
                multiplier: Multiplier::from(3),
                flow_scale: FlowScale::from(1.0),
                performance_mode: false,
                hdr_mode: false,
                experimental_present_mode: PresentMode::Vsync,
            },
        ]
    }
}

///
/// Load the configuration from the file and create a writer.
///
pub fn load_config() -> Result<(), anyhow::Error> {
    // load the configuration file
    let path = find_config_file();
    if !std::path::Path::new(&path).exists() {
        let conf = default_config();
        save_config(&conf)
            .context("Failed to create default configuration")?;
    }
    let data = std::fs::read(path)
        .context("Failed to read conf.toml")?;
    let mut config: TomlConfig = toml::from_slice(&data)
        .context("Failed to parse conf.toml")?;

    // remove duplicate entries
    config.game.sort_by_key(|e| e.exe.clone());
    config.game.dedup_by_key(|e| e.exe.clone());
    config.game.retain(|e| !e.exe.is_empty());

    // create the configuration writer thread
    let (tx, rx) = std::sync::mpsc::channel::<()>();
    CONFIG.set(Arc::new(RwLock::new(config)))
        .ok().context("Failed to set configuration state")?;
    CONFIG_WRITER.set(tx)
        .ok().context("Failed to set configuration writer")?;

    std::thread::spawn(move || {
        let config = CONFIG.get().unwrap();
        loop {
            // wait for a signal to write the configuration
            if let Err(_) = rx.recv() {
                break;
            }

            // wait a bit to avoid excessive writes
            std::thread::sleep(std::time::Duration::from_millis(200));

            // empty the channel
            while rx.try_recv().is_ok() {}

            // write the configuration
            if let Ok(config) = config.try_read() {
                if let Err(e) = save_config(&config) {
                    eprintln!("Failed to save configuration: {}", e);
                }
            } else {
                eprintln!("Failed to read configuration state");
            }
        }
    });
    Ok(())
}

///
/// Get a snapshot of the current configuration
///
pub fn get_config() -> Result<TomlConfig, anyhow::Error> {
    let conf = CONFIG.get()
        .expect("Configuration not loaded")
        .try_read()
        .map(|config| config.clone());
    if let Ok(config) = conf {
        return Ok(config)
    }

    anyhow::bail!("Failed to read configuration state")
}

///
/// Safely edit the configuration.
///
pub fn edit_config<F>(f: F) -> Result<(), anyhow::Error>
where
    F: FnOnce(&mut TomlConfig)
{
    let mut config = CONFIG.get()
        .expect("Configuration not loaded")
        .write()
        .map_err(|_| anyhow::anyhow!("Failed to acquire write lock on configuration"))?;

    f(&mut config);

    CONFIG_WRITER.get().unwrap().send(())
        .context("Failed to send configuration update signal")
}

///
/// Save the configuration to the file
///
/// # Arguments
///
/// `config` - The configuration to save
///
pub fn save_config(config: &TomlConfig) -> Result<(), anyhow::Error> {
    let path = find_config_file();

    let parent = std::path::Path::new(&path).parent()
        .context("Failed to get parent directory of config path")?;
    std::fs::create_dir_all(parent)
        .context("Failed to create config directory")?;

    let data = toml::to_string(config)
        .context("Failed to serialize conf.toml")?;
    std::fs::write(path, data)
        .context("Failed to write conf.toml")?;
    Ok(())
}

///
/// Load MangoHUD configuration from file
///
pub fn load_mangohud_config() -> Result<TomlMangoHud, anyhow::Error> {
    let path = find_mangohud_config_file();
    
    if !std::path::Path::new(&path).exists() {
        return Ok(TomlMangoHud::default());
    }

    let content = std::fs::read_to_string(&path)
        .context("Failed to read MangoHud.conf")?;

    let mut fps_limit = 60i64; // default value
    let mut found_fps_limit = false;

    for line in content.lines() {
        let line = line.trim();
        
        // Check for commented fps_limit (disabled)
        if line.starts_with("#fps_limit=") {
            fps_limit = 0; // Return 0 to indicate no limit (commented)
            found_fps_limit = true;
        }
        // Check for uncommented fps_limit (enabled)
        else if line.starts_with("fps_limit=") {
            if let Some(value_str) = line.strip_prefix("fps_limit=") {
                if let Ok(value) = value_str.parse::<i64>() {
                    fps_limit = value;
                    found_fps_limit = true;
                }
            }
        }
    }

    // If no fps_limit line was found, use default
    if !found_fps_limit {
        fps_limit = 60;
    }

    Ok(TomlMangoHud { fps_limit })
}

///
/// Save MangoHUD configuration to file
///
pub fn save_mangohud_config(mangohud_config: &TomlMangoHud) -> Result<(), anyhow::Error> {
    let path = find_mangohud_config_file();
    
    if !std::path::Path::new(&path).exists() {
        return Err(anyhow::anyhow!("MangoHud.conf file does not exist at {}", path));
    }

    let content = std::fs::read_to_string(&path)
        .context("Failed to read MangoHud.conf")?;

    let mut new_content = String::new();
    let mut fps_limit_updated = false;

    for line in content.lines() {
        let trimmed_line = line.trim();
        
        // Handle both commented and uncommented fps_limit lines
        if trimmed_line.starts_with("fps_limit=") || trimmed_line.starts_with("#fps_limit=") {
            if mangohud_config.fps_limit == 0 {
                // Comment the line to disable fps limit
                new_content.push_str(&format!("#fps_limit={}\n", 60)); // Use 60 as default when commenting
            } else {
                // Uncomment and set the value
                new_content.push_str(&format!("fps_limit={}\n", mangohud_config.fps_limit));
            }
            fps_limit_updated = true;
        } else {
            new_content.push_str(line);
            new_content.push('\n');
        }
    }

    // If fps_limit wasn't found in the file, add it
    if !fps_limit_updated {
        if mangohud_config.fps_limit == 0 {
            new_content.push_str("#fps_limit=60\n");
        } else {
            new_content.push_str(&format!("fps_limit={}\n", mangohud_config.fps_limit));
        }
    }

    let parent = std::path::Path::new(&path).parent()
        .context("Failed to get parent directory of MangoHud config path")?;
    std::fs::create_dir_all(parent)
        .context("Failed to create MangoHud config directory")?;

    std::fs::write(&path, new_content)
        .context("Failed to write MangoHud.conf")?;

    Ok(())
}

use std::sync::{Arc, OnceLock, RwLock};

use anyhow::Context;

pub mod structs;
pub use structs::*;

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

///
/// Load the configuration from the file and create a writer.
///
pub fn load_config() -> Result<(), anyhow::Error> {
    // load the configuration file
    let path = find_config_file();
    let data = std::fs::read(path)
        .context("Failed to read conf.toml")?;
    let config: TomlConfig = toml::from_slice(&data)
        .context("Failed to parse conf.toml")?;
    CONFIG.set(Arc::new(RwLock::new(config)))
        .ok().context("Failed to set configuration state")?;

    // create the configuration writer thread
    let (tx, rx) = std::sync::mpsc::channel::<()>();
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
                } else {
                    eprintln!("Configuration saved successfully");
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

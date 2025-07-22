use serde::{Deserialize, Serialize};
use std::{fs, io};
use std::path::PathBuf;
use toml;
use dirs;

use crate::utils::round_to_2_decimals; // Import from utils module

// --- Configuration Data Structures ---

#[derive(Debug, Default, Serialize, Deserialize, Clone)]
pub struct Config {
    pub version: u32, // Made public to be accessible from main.rs
    #[serde(flatten)] // Flatten this struct into the parent, controlling order
    pub ordered_global: OrderedGlobalConfig,
    #[serde(default)]
    pub game: Vec<GameProfile>,
}

// Helper struct to control the serialization order of global config
#[derive(Debug, Default, Serialize, Deserialize, Clone)]
pub struct OrderedGlobalConfig {
    #[serde(default, skip_serializing_if = "Option::is_none")] // Only serialize if Some
    pub global: Option<GlobalConfig>,
}

#[derive(Debug, Default, Serialize, Deserialize, Clone)]
pub struct GlobalConfig {
    #[serde(default, skip_serializing_if = "Option::is_none")] // Only serialize if Some
    pub dll: Option<String>,
}

#[derive(Debug, Serialize, Deserialize, Clone)]
#[serde(default)]
pub struct GameProfile {
    pub exe: String,
    pub multiplier: u32,
    #[serde(serialize_with = "serialize_flow_scale", deserialize_with = "deserialize_flow_scale")]
    pub flow_scale: f32,
    pub performance_mode: bool,
    pub hdr_mode: bool,
    pub experimental_present_mode: String,
}

// Default values for a new game profile
impl Default for GameProfile {
    fn default() -> Self {
        GameProfile {
            exe: String::new(),
            multiplier: 1, // Default to "off" (1)
            flow_scale: round_to_2_decimals(0.7),
            performance_mode: true,
            hdr_mode: false,
            experimental_present_mode: "vsync".to_string(),
        }
    }
}

// Custom serde functions to ensure flow_scale is always rounded
fn serialize_flow_scale<S>(value: &f32, serializer: S) -> Result<S::Ok, S::Error>
where
    S: serde::Serializer,
{
    // Force to 2 decimal places and serialize as a precise decimal
    let rounded = round_to_2_decimals(*value);
    let formatted = format!("{:.2}", rounded);
    let precise_value: f64 = formatted.parse().unwrap_or(*value as f64);
    serializer.serialize_f64(precise_value)
}

fn deserialize_flow_scale<'de, D>(deserializer: D) -> Result<f32, D::Error>
where
    D: serde::Deserializer<'de>,
{
    use serde::Deserialize;
    let value = f64::deserialize(deserializer)?;
    Ok(round_to_2_decimals(value as f32))
}

// --- Configuration File Handling Functions ---

pub fn get_config_path() -> Result<PathBuf, io::Error> {
    let config_dir = dirs::config_dir()
        .ok_or_else(|| io::Error::new(io::ErrorKind::NotFound, "Could not find config directory"))?
        .join("lsfg-vk");
    
    fs::create_dir_all(&config_dir)?; // Ensure directory exists
    println!("Config directory: {:?}", config_dir);
    Ok(config_dir.join("conf.toml"))
}


pub fn load_config() -> Result<Config, io::Error> {
    let config_path = get_config_path()?;
    println!("Attempting to load config from: {:?}", config_path);
    if config_path.exists() {
        let contents = fs::read_to_string(&config_path)?;
        println!("Successfully read config contents ({} bytes).", contents.len());
        // Load configuration with default values when the format is invalid
        let mut config: Config = toml::from_str(&contents).unwrap_or_else(|_| Config::default());

        // Old way to load config
        // let mut config: Config = toml::from_str(&contents).map_err(|e| {
        //     io::Error::new(
        //         io::ErrorKind::InvalidData,
        //         format!("Failed to parse TOML: {}", e),
        //     )
        // })?;

        
        // Clean up any floating point precision issues in existing configs
        let mut needs_save = false;
        for profile in &mut config.game {
            let original = profile.flow_scale;
            profile.flow_scale = round_to_2_decimals(profile.flow_scale);
            if (original - profile.flow_scale).abs() > f32::EPSILON {
                needs_save = true;
            }
        }
        
        // Save the cleaned config if we made changes
        if needs_save {
            let _ = save_config(&config);
        }
        
        Ok(config)
    } else {
        println!("Config file not found at {:?}, creating default.", config_path);
        Ok(Config { version: 1, ordered_global: OrderedGlobalConfig { global: None }, game: Vec::new() })
    }
}

pub fn save_config(config: &Config) -> Result<(), io::Error> {
    let config_path = get_config_path()?;
    println!("Attempting to save config to: {:?}", config_path);
    let toml_string = toml::to_string_pretty(config)
        .map_err(|e| io::Error::new(io::ErrorKind::Other, format!("Failed to serialize TOML: {}", e)))?;
    fs::write(&config_path, toml_string)?;
    println!("Successfully saved config.");
    Ok(())
}

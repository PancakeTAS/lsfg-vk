use serde::{Deserialize, Serialize};

// multiplier
#[derive(Debug, Clone, Deserialize, Serialize)]
pub struct Multiplier(i64);
impl Default for Multiplier {
    fn default() -> Self { Multiplier(2) }
}
impl From<i64> for Multiplier {
    fn from(value: i64) -> Self { Multiplier(value) }
}
impl Into<f64> for Multiplier {
    fn into(self) -> f64 { self.0 as f64 }
}

// flow scale
#[derive(Debug, Clone, Deserialize, Serialize)]
pub struct FlowScale(f64);
impl Default for FlowScale {
    fn default() -> Self { FlowScale(1.0) }
}
impl From<f64> for FlowScale {
    fn from(value: f64) -> Self { FlowScale(value) }
}
impl Into<f64> for FlowScale {
    fn into(self) -> f64 { self.0 }
}

// present mode
#[derive(Debug, Clone, Deserialize, Serialize)]
pub enum PresentMode {
    #[serde(rename = "fifo", alias = "vsync")]
    Vsync,
    #[serde(rename = "immediate")]
    Immediate,
    #[serde(rename = "mailbox")]
    Mailbox,
}
impl Default for PresentMode {
    fn default() -> Self { PresentMode::Vsync }
}
impl From<i64> for PresentMode {
    fn from(value: i64) -> Self {
        match value {
            0 => PresentMode::Vsync,
            1 => PresentMode::Mailbox,
            2 => PresentMode::Immediate,
            _ => PresentMode::Vsync,
        }
    }
}
impl Into<u32> for PresentMode {
    fn into(self) -> u32 {
        match self {
            PresentMode::Vsync => 0,
            PresentMode::Mailbox => 1,
            PresentMode::Immediate => 2,
        }
    }
}

/// Global configuration for the application
#[derive(Debug, Clone, Default, Deserialize, Serialize)]
pub struct TomlGlobal {
    pub dll: Option<String>
}

/// Game-specific configuration
#[derive(Debug, Default, Clone, Deserialize, Serialize)]
pub struct TomlGame {
    pub exe: String,

    #[serde(default)]
    pub multiplier: Multiplier,
    #[serde(default)]
    pub flow_scale: FlowScale,
    #[serde(default)]
    pub performance_mode: bool,
    #[serde(default)]
    pub hdr_mode: bool,
    #[serde(default)]
    pub experimental_present_mode: PresentMode
}

/// Main configuration structure
#[derive(Debug, Clone, Deserialize, Serialize)]
pub struct TomlConfig {
    pub version: i64,
    #[serde(default)]
    pub global: TomlGlobal,
    #[serde(default)]
    pub game: Vec<TomlGame>
}

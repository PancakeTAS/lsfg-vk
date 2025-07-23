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

// flow scale
#[derive(Debug, Clone, Deserialize, Serialize)]
pub struct FlowScale(f64);
impl Default for FlowScale {
    fn default() -> Self { FlowScale(1.0) }
}
impl From<f64> for FlowScale {
    fn from(value: f64) -> Self { FlowScale(value) }
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

/// Global configuration for the application
#[derive(Debug, Clone, Default, Deserialize, Serialize)]
pub struct TomlGlobal {
    pub dll: Option<String>
}

/// Game-specific configuration
#[derive(Debug, Clone, Deserialize, Serialize)]
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

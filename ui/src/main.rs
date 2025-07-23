mod ui;

const APP_ID: &str = "gay.pancake.lsfg-vk.ConfigurationUi";

fn main() {
    let app = ui::App::new(APP_ID)
        .expect("Failed to create application");
    app.run()
}

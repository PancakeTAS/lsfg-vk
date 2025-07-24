use adw::subclass::prelude::ObjectSubclassIsExt;
use gtk::prelude::{ButtonExt, ListBoxRowExt, RangeExt, WidgetExt};

use crate::{config, ui::entry_handler, wrapper::{pane, entry}, STATE};

///
/// Register signals for adding and selecting presets.
///
pub fn register_signals(sidebar_: &pane::PaneSidebar, main: pane::PaneMain) {
    // activate signal
    let state = STATE.get().unwrap().clone();
    sidebar_.imp().profiles.connect_row_activated(move |_, entry| {
        // find config entry by index
        let index = entry.index() as usize;
        let config = config::get_config();
        if config.is_err() {
            return;
        }
        let config = config.unwrap();
        let conf = config.game[index].clone();

        // update main pane
        let main = main.imp();
        let multiplier = main.multiplier.imp();
        let flow_scale = main.flow_scale.imp();
        let performance_mode = main.performance_mode.imp();
        let hdr_mode = main.hdr_mode.imp();
        let experimental_present_mode = main.experimental_present_mode.imp();

        // (lock state early, so the ui update doesn't override the config)
        if let Ok(mut state) = state.write() {
            multiplier.number.set_value(conf.multiplier.into());
            flow_scale.slider.set_value(Into::<f64>::into(conf.flow_scale) * 100.0);
            performance_mode.switch.set_state(conf.performance_mode);
            hdr_mode.switch.set_state(conf.hdr_mode);
            experimental_present_mode.dropdown.set_selected(conf.experimental_present_mode.into());

            // update state
            state.selected_game = Some(index);
        }
    });

    // create signal
    let sidebar = sidebar_.clone();
    sidebar_.imp().create.connect_clicked(move |_| {
        // ensure no config entry with the same name exist
        let config = config::get_config().unwrap();
        if config.game.iter().any(|e| e.exe == "new profile") {
            return;
        }

        // create config entry
        let mut conf_entry = config::TomlGame::default();
        conf_entry.exe = "new profile".to_string();
        let _ = config::edit_config(|config| {
            config.game.push(conf_entry.clone());
        });

        // add entry to sidebar
        let entry = entry::Entry::new();
        entry.set_exe(conf_entry.exe);
        entry_handler::add_entry(entry.clone(), sidebar.imp().profiles.clone());

        // select the new entry
        entry.activate();
    });
}

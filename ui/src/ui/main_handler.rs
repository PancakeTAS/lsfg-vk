use adw::subclass::prelude::ObjectSubclassIsExt;
use gtk::{gio::{self, prelude::FileExt}, glib::object::CastNone, prelude::{ButtonExt, EditableExt, GtkWindowExt, ListBoxRowExt, RangeExt, WidgetExt}};

use crate::{config, utils, wrapper::{entry, pane, popup}, STATE};

// update the currently selected game configuration
fn update_game<F: FnOnce(&mut config::TomlGame)>(update: F) {
    if let Ok(state) = STATE.get().unwrap().try_read() {
        if let Some(selected_game) = state.selected_game {
            let _ = config::edit_config(|config| {
                update(&mut config.game[selected_game])
            });
        }
    }
}

///
/// Register signals for preset preferences.
///
pub fn register_signals(sidebar_: pane::PaneSidebar, main: &pane::PaneMain) {
    let main = main.imp();
    let exe = main.preset_name.imp();
    let multiplier = main.multiplier.imp();
    let flow_scale = main.flow_scale.imp();
    let performance_mode = main.performance_mode.imp();
    let hdr_mode = main.hdr_mode.imp();
    let experimental_present_mode = main.experimental_present_mode.imp();

    // preset opts
    let sidebar = sidebar_.clone();
    exe.entry.connect_changed(move |entry| {
        let mut exe = entry.text().to_string();
        if exe.trim().is_empty() {
            exe = "new preset".to_string();
        }

        // rename list entry
        let row = sidebar.imp().profiles.selected_row()
            .and_downcast::<entry::Entry>().unwrap();
        row.set_exe(exe.clone());

        // update the game configuration
        update_game(|conf| {
            conf.exe = exe;
        });
    });
    multiplier.number.connect_value_changed(|dropdown| {
        update_game(|conf| {
            conf.multiplier = (dropdown.value() as i64).into();
        })
    });
    flow_scale.slider.connect_value_changed(|slider| {
        update_game(|conf| {
            conf.flow_scale = (slider.value() / 100.0).into();
        });
    });
    performance_mode.switch.connect_state_notify(|switch| {
        update_game(|conf| {
            conf.performance_mode = switch.state();
        });
    });
    hdr_mode.switch.connect_state_notify(|switch| {
        update_game(|conf| {
            conf.hdr_mode = switch.state();
        });
    });
    experimental_present_mode.dropdown.connect_selected_notify(|dropdown| {
        update_game(|conf| {
            conf.experimental_present_mode = match dropdown.selected() {
                0 => config::PresentMode::Vsync,
                1 => config::PresentMode::Mailbox,
                2 => config::PresentMode::Immediate,
                _ => config::PresentMode::Vsync,
            };
        });
    });

    // global opts
    let dll = main.dll.imp();
    dll.entry.connect_changed(|entry| {
        let _ = config::edit_config(|config| {
            let text = entry.text().to_string();
            if text.trim().is_empty() {
                config.global.dll = None;
            } else {
                config.global.dll = Some(text);
            }
        });
    });

    // utility buttons
    let entry = dll.entry.clone();
    dll.btn.connect_clicked(move |btn| {
        let dialog = gtk::FileDialog::new();
        dialog.set_title("Select Lossless.dll");

        let filter = gtk::FileFilter::new();
        filter.set_name(Some("Lossless.dll"));
        filter.add_pattern("Lossless.dll");

        let filters = gio::ListStore::new::<gtk::FileFilter>();
        filters.append(&filter);
        dialog.set_filters(Some(&filters));
        dialog.set_default_filter(Some(&filter));

        let window = btn.root()
            .and_downcast::<gtk::Window>()
            .unwrap();
        let entry = entry.clone();
        dialog.open(Some(&window), gio::Cancellable::NONE, move |result| {
            if result.is_err() || result.as_ref().unwrap().path().is_none() {
                return;
            }

            let path = result.unwrap().path().unwrap();
            let path_str = path.to_string_lossy().to_string();

            entry.set_text(&path_str);
            let _ = config::edit_config(|config| {
                config.global.dll = Some(path_str);
            });
        });
    });

    let entry = exe.entry.clone();
    exe.btn.connect_clicked(move |btn| {
        let window = btn.root()
            .and_downcast::<gtk::ApplicationWindow>()
            .unwrap()
            .application()
            .unwrap();
        let picker = popup::ProcessPicker::new();
        picker.set_application(Some(&window));

        let list = picker.imp().processes.clone();
        let processes = utils::find_vulkan_processes().unwrap_or_default();
        for process in &processes {
            let entry = popup::ProcessEntry::new();
            entry.set_exe(process.0.clone());
            list.append(&entry);
        }

        let entry = entry.clone();
        let picker_ = picker.clone();
        picker.imp().processes.connect_row_activated(move |_, row| {
            let comm_str = processes[row.index() as usize].1.clone();

            entry.set_text(&comm_str);
            update_game(|conf| {
                conf.exe = comm_str;
            });

            picker_.close();
        });

        let picker_ = picker.clone();
        picker.imp().close.connect_clicked(move |_| {
            picker_.close();
        });

        picker.present();
    });
}

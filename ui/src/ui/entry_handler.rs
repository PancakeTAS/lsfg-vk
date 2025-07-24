use adw::subclass::prelude::ObjectSubclassIsExt;
use gtk::{gio, glib::object::CastNone, prelude::{ButtonExt, ListBoxRowExt, WidgetExt}};

use crate::{config, wrapper::entry, STATE};

///
/// Register signals for removing presets when adding a new entry.
///
pub fn add_entry(entry_: entry::Entry, profiles_: gtk::ListBox) {
    let entry = entry_.clone();
    let profiles = profiles_.clone();
    entry_.imp().delete.connect_clicked(move |btn| {
        // prompt for confirmation
        let dialog = gtk::AlertDialog::builder()
            .message("Delete Profile")
            .detail("Are you sure you want to delete this profile?")
            .buttons(vec!["Cancel".to_string(), "Delete".to_string()])
            .cancel_button(0)
            .default_button(1)
            .modal(true)
            .build();
        let window = btn.root()
            .and_downcast::<gtk::Window>()
            .expect("Button root is not a Window");

        let profiles = profiles.clone();
        let entry = entry.clone();
        dialog.choose(Some(&window), None::<&gio::Cancellable>, move |result| {
            if result.is_err() || result.unwrap() != 1 {
                return;
            }

            // remove config entry
            let _ = config::edit_config(|config| {
                config.game.remove(entry.index() as usize);
            });

            // remove ui entry
            profiles.remove(&entry);

            // select next entry
            let idx = entry.index() as usize;
            if idx == 0 {
                let state = STATE.get().unwrap().clone();
                if let Ok(mut state) = state.write() {
                    state.selected_game = None;
                }
                return;
            }

            profiles.row_at_index(0).unwrap().activate();
        });
    });

    profiles_.append(&entry_);
}

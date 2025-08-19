use adw::subclass::prelude::ObjectSubclassIsExt;
use gtk::{gio, glib::{self, object::CastNone}, prelude::{ButtonExt, ListBoxRowExt, WidgetExt, BoxExt, EntryExt, GtkWindowExt, WindowExt, EditableExt}};

use crate::{config, wrapper::entry, STATE};

///
/// Register signals for removing presets when adding a new entry.
///
pub fn add_entry(entry_: entry::Entry, profiles_: gtk::ListBox) {
    let entry = entry_.clone();
    let profiles = profiles_.clone();
    
    // Edit button handler
    entry_.imp().edit.connect_clicked(glib::clone!(@weak entry, @weak profiles => move |btn| {
        // Show an entry dialog for editing the profile name
        let dialog = gtk::AlertDialog::builder()
            .message("Edit Profile Name")
            .detail("Enter a custom name for this profile:")
            .modal(true)
            .build();
        
        // Create an entry widget for the dialog
        let entry_widget = gtk::Entry::new();
        if let Some(current_name) = entry.name() {
            entry_widget.set_text(&current_name);
        } else {
            entry_widget.set_text(&entry.exe());
        }
        entry_widget.set_placeholder_text(Some(&entry.exe()));
        
        // Note: GTK4's AlertDialog doesn't directly support custom widgets
        // We'll use a simple input dialog approach instead
        let window = btn.root()
            .and_downcast::<gtk::Window>()
            .expect("Button root is not a Window");
        
        // Create a simple dialog window for name editing
        let dialog_window = gtk::Window::new();
        dialog_window.set_title("Edit Profile Name");
        dialog_window.set_modal(true);
        dialog_window.set_transient_for(Some(&window));
        dialog_window.set_default_size(400, 150);
        
        let vbox = gtk::Box::new(gtk::Orientation::Vertical, 12);
        vbox.set_margin_start(20);
        vbox.set_margin_end(20);
        vbox.set_margin_top(20);
        vbox.set_margin_bottom(20);
        
        let label = gtk::Label::new(Some("Enter a custom name for this profile:"));
        label.set_halign(gtk::Align::Start);
        vbox.append(&label);
        
        let name_entry = gtk::Entry::new();
        if let Some(current_name) = entry.name() {
            name_entry.set_text(&current_name);
        } else {
            name_entry.set_text(&entry.exe());
        }
        name_entry.set_placeholder_text(Some(&entry.exe()));
        vbox.append(&name_entry);
        
        let button_box = gtk::Box::new(gtk::Orientation::Horizontal, 6);
        button_box.set_halign(gtk::Align::End);
        
        let cancel_btn = gtk::Button::with_label("Cancel");
        let save_btn = gtk::Button::with_label("Save");
        save_btn.add_css_class("suggested-action");
        
        button_box.append(&cancel_btn);
        button_box.append(&save_btn);
        vbox.append(&button_box);
        
        dialog_window.set_child(Some(&vbox));
        
        // Handle cancel
        cancel_btn.connect_clicked(glib::clone!(@weak dialog_window => move |_| {
            dialog_window.close();
        }));
        
        // Handle save
        save_btn.connect_clicked(glib::clone!(@weak entry, @weak dialog_window, @weak name_entry => move |_| {
            let new_name = name_entry.text().to_string();
            
            // Update the entry's name
            if new_name.trim().is_empty() || new_name == entry.exe() {
                entry.set_name(None);
            } else {
                entry.set_name(Some(new_name.clone()));
            }
            
            // Update config
            let _ = config::edit_config(|config| {
                let index = entry.index() as usize;
                if index < config.game.len() {
                    if new_name.trim().is_empty() || new_name == entry.exe() {
                        config.game[index].name = None;
                    } else {
                        config.game[index].name = Some(new_name);
                    }
                }
            });
            
            dialog_window.close();
        }));
        
        // Handle Enter key
        name_entry.connect_activate(glib::clone!(@weak save_btn => move |_| {
            save_btn.emit_clicked();
        }));
        
        dialog_window.present();
        name_entry.grab_focus();
    }));
    
    // Delete button handler
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
        dialog.choose(Some(&window), gio::Cancellable::NONE, move |result| {
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
            let state = STATE.get().unwrap().clone();
            if let Ok(mut state) = state.write() {
                state.selected_game = None;
            }

            if let Some(entry) = profiles.row_at_index(0) {
                entry.activate();
            }
        });
    });

    profiles_.append(&entry_);
}

use gtk::prelude::*;
use gtk::{glib, Label, Switch, Entry, Box, Orientation};
use libadwaita::prelude::*;
use libadwaita::{ApplicationWindow, PreferencesGroup, PreferencesPage, PreferencesWindow, ActionRow};
use std::rc::Rc;
use std::cell::RefCell;

use crate::app_state::AppState;

pub fn create_settings_window(parent: &ApplicationWindow, app_state: Rc<RefCell<AppState>>) -> PreferencesWindow {
    let settings_window = PreferencesWindow::builder()
        .title("Settings")
        .transient_for(parent)
        .modal(true)
        .search_enabled(false)
        .default_width(450) // Set default width
        .default_height(300) // Set default height
        .build();

    let page = PreferencesPage::builder()
        .title("General")
        .icon_name("preferences-system-symbolic")
        .build();

    let group = PreferencesGroup::builder()
        .title("Global Settings")
        .build();

    // --- Custom DLL Toggle and Path (Programmatically created) ---
    let custom_dll_switch = Switch::builder()
        .halign(gtk::Align::End)
        .valign(gtk::Align::Center)
        .build();

    let custom_dll_path_entry = Entry::builder()
        .placeholder_text("/path/to/Lossless.dll")
        .hexpand(true)
        .build();

    let custom_dll_row = ActionRow::builder()
        .title("Custom Path to Lossless.dll")
        .build();
    custom_dll_row.add_suffix(&custom_dll_switch);
    custom_dll_row.set_activatable_widget(Some(&custom_dll_switch));

    let custom_dll_box = Box::builder()
        .orientation(Orientation::Vertical)
        .spacing(6)
        .margin_top(6)
        .margin_bottom(6)
        .build();
    custom_dll_box.append(&custom_dll_row);
    custom_dll_box.append(&custom_dll_path_entry);

    group.add(&custom_dll_box); // Add the box directly to the group

    // Initial state setup for Custom DLL
    let current_dll_path = app_state.borrow().config.ordered_global.global.as_ref()
        .and_then(|g| g.dll.clone());

    if let Some(path) = current_dll_path {
        custom_dll_switch.set_active(true);
        custom_dll_path_entry.set_text(&path);
        custom_dll_path_entry.set_visible(true);
    } else {
        custom_dll_switch.set_active(false);
        custom_dll_path_entry.set_visible(false);
    }

    // Connect switch to show/hide entry and update config
    let app_state_clone_switch = app_state.clone();
    let custom_dll_path_entry_clone = custom_dll_path_entry.clone();
    custom_dll_switch.connect_state_set(move |_sw, active| {
        custom_dll_path_entry_clone.set_visible(active);
        let mut state = app_state_clone_switch.borrow_mut();
        if active {
            // If activating, ensure global config exists and set DLL path
            let current_path = custom_dll_path_entry_clone.text().to_string();
            state.config.ordered_global.global.get_or_insert_with(Default::default).dll = Some(current_path);
        } else {
            // If deactivating, set DLL path to None
            if let Some(global_config) = state.config.ordered_global.global.as_mut() {
                global_config.dll = None;
            }
        }
        glib::Propagation::Proceed
    });

    // Connect entry to update config
    let app_state_clone_entry = app_state.clone();
    let custom_dll_switch_clone = custom_dll_switch.clone();
    custom_dll_path_entry.connect_changed(move |entry| {
        let mut state = app_state_clone_entry.borrow_mut();
        if custom_dll_switch_clone.is_active() {
            let path = entry.text().to_string();
            if !path.is_empty() {
                state.config.ordered_global.global.get_or_insert_with(Default::default).dll = Some(path);
            } else {
                // If path is cleared, set dll to None
                if let Some(global_config) = state.config.ordered_global.global.as_mut() {
                    global_config.dll = None;
                }
            }
        }
    });

    // Save button for settings
    let save_settings_button = gtk::Button::builder()
        .label("Save Global Settings")
        .halign(gtk::Align::End)
        .margin_end(12)
        .margin_bottom(12)
        .margin_top(12)
        .build();
    
    // Create a box to hold the feedback label
    let feedback_container_box = Box::builder()
        .orientation(Orientation::Vertical)
        .halign(gtk::Align::End)
        .margin_end(12)
        .margin_bottom(12)
        .build();

    group.add(&save_settings_button); // Add button first
    group.add(&feedback_container_box); // Then add the container for feedback

    let app_state_clone_save = app_state.clone();
    let feedback_container_box_clone = feedback_container_box.clone(); // Clone for the closure
    save_settings_button.connect_clicked(move |_| {
        let state = app_state_clone_save.borrow_mut(); // Removed 'mut'
        state.save_current_config();

        let feedback_label = Label::new(Some("Saved!"));
        feedback_label.set_halign(gtk::Align::End);
        feedback_label.set_margin_end(12);
        feedback_label.set_margin_bottom(12);
        
        // Append to the dedicated feedback container box
        feedback_container_box_clone.append(&feedback_label);

        glib::timeout_add_local(std::time::Duration::new(2, 0), {
            let feedback_label_clone = feedback_label.clone(); // Clone for the timeout
            let feedback_container_box_clone_for_remove = feedback_container_box_clone.clone(); // Clone for the timeout
            move || {
                feedback_container_box_clone_for_remove.remove(&feedback_label_clone);
                glib::ControlFlow::Break
            }
        });
    });
    
    page.add(&group);
    settings_window.add(&page);

    settings_window
}

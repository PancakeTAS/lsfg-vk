use gtk::prelude::*;
use gtk::{glib, MessageDialog};
use libadwaita::ApplicationWindow;
use std::cell::RefCell;
use std::rc::Rc;

use crate::config::{Config, save_config};
use crate::utils::round_to_2_decimals;

#[allow(dead_code)]
pub struct AppState {
    pub config: Config,
    pub selected_profile_index: Option<usize>,
    // Store references to the UI widgets for easy access and updates
    pub main_window: ApplicationWindow,
    pub sidebar_list_box: gtk::ListBox,
    pub multiplier_dropdown: gtk::DropDown,
    pub flow_scale_entry: gtk::Entry,
    pub performance_mode_switch: gtk::Switch,
    pub hdr_mode_switch: gtk::Switch,
    pub experimental_present_mode_dropdown: gtk::DropDown,
    pub save_button: gtk::Button,
    pub main_settings_box: gtk::Box,
    pub main_stack: gtk::Stack,
    // Store SignalHandlerIds to block/unblock signals
    pub multiplier_dropdown_handler_id: Option<glib::SignalHandlerId>,
    pub flow_scale_entry_handler_id: Option<glib::SignalHandlerId>,
    pub performance_mode_switch_handler_id: Option<glib::SignalHandlerId>,
    pub hdr_mode_switch_handler_id: Option<glib::SignalHandlerId>,
    pub experimental_present_mode_dropdown_handler_id: Option<glib::SignalHandlerId>,
}

impl AppState {
    // Saves the current configuration to the TOML file
    pub fn save_current_config(&self) {
        if let Err(e) = save_config(&self.config) {
            eprintln!("Failed to save config: {}", e);
            // In a real app, you'd show a user-friendly error dialog here
        }
    }

    // Updates the main window UI with data from the currently selected profile
    pub fn update_main_window_from_profile(&self) {
        if let Some(index) = self.selected_profile_index {
            if let Some(profile) = self.config.game.get(index) {
                // Temporarily block signals to prevent re-entrancy
                let _guard_mult = self.multiplier_dropdown_handler_id.as_ref().map(|id| self.multiplier_dropdown.block_signal(id));
                let _guard_flow = self.flow_scale_entry_handler_id.as_ref().map(|id| self.flow_scale_entry.block_signal(id));
                let _guard_perf = self.performance_mode_switch_handler_id.as_ref().map(|id| self.performance_mode_switch.block_signal(id));
                let _guard_hdr = self.hdr_mode_switch_handler_id.as_ref().map(|id| self.hdr_mode_switch.block_signal(id));
                let _guard_exp = self.experimental_present_mode_dropdown_handler_id.as_ref().map(|id| self.experimental_present_mode_dropdown.block_signal(id));

                // Update Multiplier Dropdown
                let multiplier_str = match profile.multiplier {
                    1 => "off".to_string(),
                    _ => profile.multiplier.to_string(),
                };
                if let Some(pos) = self.multiplier_dropdown.model().and_then(|model| {
                    let list_model = model.downcast_ref::<gtk::StringList>()?;
                    (0..list_model.n_items()).find(|&i| list_model.string(i).map_or(false, |s| s.as_str() == multiplier_str))
                }) {
                    self.multiplier_dropdown.set_selected(pos);
                }

                // Update Flow Scale Entry (round to avoid floating point display issues)
                let rounded_flow_scale = round_to_2_decimals(profile.flow_scale);
                self.flow_scale_entry.set_text(&format!("{:.2}", rounded_flow_scale));

                // Update Performance Mode Switch
                self.performance_mode_switch.set_active(profile.performance_mode);

                // Update HDR Mode Switch
                self.hdr_mode_switch.set_active(profile.hdr_mode);

                // Update Experimental Present Mode Dropdown
                if let Some(pos) = self.experimental_present_mode_dropdown.model().and_then(|model| {
                    let list_model = model.downcast_ref::<gtk::StringList>()?;
                    (0..list_model.n_items()).find(|&i| list_model.string(i).map_or(false, |s| s.as_str() == profile.experimental_present_mode))
                }) {
                    self.experimental_present_mode_dropdown.set_selected(pos);
                }
                // Signal handlers are unblocked automatically when _guard_X go out of scope

                // Switch to the settings page
                self.main_stack.set_visible_child_name("settings_page");

            }
        } else {
            // Clear or disable main window elements if no profile is selected
            self.multiplier_dropdown.set_selected(0); // Default to 'off' or first item
            self.flow_scale_entry.set_text("");
            self.performance_mode_switch.set_active(false);
            self.hdr_mode_switch.set_active(false);
            self.experimental_present_mode_dropdown.set_selected(0); // Default to first item

            // Switch to the about page
            self.main_stack.set_visible_child_name("about_page");
        }
    }

    // Populates sidebar with optional app_state for button handlers
    pub fn populate_sidebar_with_handlers(&self, app_state: Option<Rc<RefCell<AppState>>>) {
        // Clear existing rows
        while let Some(child) = self.sidebar_list_box.first_child() {
            self.sidebar_list_box.remove(&child);
        }

        let mut row_to_select: Option<gtk::ListBoxRow> = None;

        for (i, profile) in self.config.game.iter().enumerate() {
            let row = gtk::ListBoxRow::new();
            
            // Create a horizontal box to hold the profile name and buttons
            let row_box = gtk::Box::builder()
                .orientation(gtk::Orientation::Horizontal)
                .spacing(8)
                .margin_start(12)
                .margin_end(12)
                .margin_top(8)
                .margin_bottom(8)
                .build();

            // Profile name label
            let label = gtk::Label::builder()
                .label(&profile.exe)
                .halign(gtk::Align::Start)
                .hexpand(true)
                .build();

            // Edit button
            let edit_button = gtk::Button::builder()
                .label("🖊")
                .css_classes(["flat", "circular"])
                .tooltip_text("Edit profile name")
                .build();

            // Remove button
            let remove_button = gtk::Button::builder()
                .label("𐄂")
                .css_classes(["flat", "circular", "destructive-action"])
                .tooltip_text("Remove profile")
                .build();

            // Add all elements to the row box
            row_box.append(&label);
            row_box.append(&edit_button);
            row_box.append(&remove_button);

            // Connect button handlers if app_state is available
            if let Some(app_state_ref) = &app_state {
                // Edit button handler
                let app_state_clone = app_state_ref.clone();
                let profile_index = i;
                edit_button.connect_clicked(move |_| {
                    let state = app_state_clone.borrow();
                    let main_window = &state.main_window;
                    
                    // Create edit dialog
                    let dialog = MessageDialog::new(
                        Some(main_window),
                        gtk::DialogFlags::MODAL,
                        gtk::MessageType::Question,
                        gtk::ButtonsType::None,
                        "Edit profile name:",
                    );
                    dialog.set_title(Some("Edit Profile"));

                    let entry = gtk::Entry::builder()
                        .placeholder_text("Profile Name")
                        .text(&state.config.game[profile_index].exe)
                        .margin_top(12)
                        .margin_bottom(12)
                        .margin_start(12)
                        .margin_end(12)
                        .build();
                    
                    dialog.content_area().append(&entry);
                    dialog.add_button("Cancel", gtk::ResponseType::Cancel);
                    dialog.add_button("Save", gtk::ResponseType::Other(1));
                    dialog.set_default_response(gtk::ResponseType::Other(1));

                    // Allow pressing Enter in the entry to trigger the "Save" button
                    let dialog_clone = dialog.clone();
                    entry.connect_activate(move |_| {
                        dialog_clone.response(gtk::ResponseType::Other(1));
                    });

                    let app_state_clone_dialog = app_state_clone.clone();
                    let entry_clone = entry.clone();
                    dialog.connect_response(move |d, response| {
                        if response == gtk::ResponseType::Other(1) {
                            let new_name = entry_clone.text().to_string();
                            if !new_name.is_empty() {
                                let mut state = app_state_clone_dialog.borrow_mut();
                                
                                // Check if profile with this name already exists (excluding current)
                                if state.config.game.iter().enumerate().any(|(idx, p)| idx != profile_index && p.exe == new_name) {
                                    let error_dialog = MessageDialog::new(
                                        Some(d),
                                        gtk::DialogFlags::MODAL,
                                        gtk::MessageType::Error,
                                        gtk::ButtonsType::Ok,
                                        "A profile with this name already exists",
                                    );
                                    error_dialog.set_title(Some("Error"));
                                    error_dialog.connect_response(move |d, _| { d.close(); });
                                    error_dialog.present();
                                    return;
                                }

                                // Update profile name
                                state.config.game[profile_index].exe = new_name;
                                state.save_current_config();
                                state.populate_sidebar_with_handlers(Some(app_state_clone_dialog.clone()));
                            }
                        }
                        d.close();
                    });
                    dialog.present();
                });

                // Remove button handler
                let app_state_clone = app_state_ref.clone();
                let profile_index = i;
                remove_button.connect_clicked(move |_| {
                    let state = app_state_clone.borrow();
                    let main_window = &state.main_window;
                    let profile_name = &state.config.game[profile_index].exe;
                    
                    // Create confirmation dialog
                    let dialog = MessageDialog::new(
                        Some(main_window),
                        gtk::DialogFlags::MODAL,
                        gtk::MessageType::Warning,
                        gtk::ButtonsType::None,
                        &format!("Are you sure you want to remove the profile '{}'?", profile_name),
                    );
                    dialog.set_title(Some("Remove Profile"));
                    dialog.add_button("Cancel", gtk::ResponseType::Cancel);
                    dialog.add_button("Remove", gtk::ResponseType::Other(1));
                    dialog.set_default_response(gtk::ResponseType::Cancel);

                    let app_state_clone_dialog = app_state_clone.clone();
                    dialog.connect_response(move |d, response| {
                        if response == gtk::ResponseType::Other(1) {
                            let mut state = app_state_clone_dialog.borrow_mut();
                            
                            // Remove the profile
                            state.config.game.remove(profile_index);
                            
                            // Update selected index if needed
                            if let Some(selected) = state.selected_profile_index {
                                if selected == profile_index {
                                    // If we removed the selected profile, select the first available or none
                                    state.selected_profile_index = if state.config.game.is_empty() { None } else { Some(0) };
                                } else if selected > profile_index {
                                    // Adjust index if we removed a profile before the selected one
                                    state.selected_profile_index = Some(selected - 1);
                                }
                            }
                            
                            state.save_current_config();
                            state.populate_sidebar_with_handlers(Some(app_state_clone_dialog.clone()));
                            drop(state);
                            
                            // Update main window
                            app_state_clone_dialog.borrow().update_main_window_from_profile();
                        }
                        d.close();
                    });
                    dialog.present();
                });
            }

            row.set_child(Some(&row_box));
            self.sidebar_list_box.append(&row);

            // Mark the row to be selected later
            if self.selected_profile_index == Some(i) {
                row_to_select = Some(row.clone()); // Clone the row to store it
            }
        }

        // Perform selection in a separate idle callback
        if let Some(row) = row_to_select {
            let list_box_clone = self.sidebar_list_box.clone();
            glib::idle_add_local(move || {
                list_box_clone.select_row(Some(&row));
                glib::ControlFlow::Break
            });
        }
    }
}

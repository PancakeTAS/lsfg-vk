use gtk::prelude::*;
use gtk::{glib, CssProvider, Builder, Label};
use libadwaita::ApplicationWindow;
use libadwaita::prelude::AdwApplicationWindowExt;
use std::cell::RefCell;
use std::rc::Rc;

// Import modules
mod config;
mod app_state;
mod utils;
mod settings_window;

use config::load_config;
use app_state::AppState;
use utils::round_to_2_decimals;
use config::OrderedGlobalConfig;

fn main() -> glib::ExitCode {
    let application = libadwaita::Application::builder()
        .application_id("com.cali666.lsfg-vk-ui")
        .build();
    
    // Set the desktop file name for proper GNOME integration
    glib::set_application_name("LSFG-VK UI");
    glib::set_prgname(Some("lsfg-vk-ui"));

    application.connect_startup(move |_app| {
        // Load CSS for sidebar background
        let provider = CssProvider::new();
        provider.load_from_data(&format!(
            ".settings-icon-button {{
                font-size: 1.4rem;
            }}

            .sidebar {{
                background-color: @theme_bg_color;
            }}
            
            .sidebar-content {{
                background-color: shade(@theme_bg_color, {});
                color: @theme_fg_color;
                padding: 12px;
            }}\n
            .linked-button-box {{
                margin-top: 12px;
                margin-bottom: 12px;
            }}",
            0.95
        ));
        gtk::style_context_add_provider_for_display(
            &gtk::gdk::Display::default().expect("Could not connect to a display."),
            &provider,
            gtk::STYLE_PROVIDER_PRIORITY_APPLICATION,
        );

        // Set up icon theme for the application icon
        if let Some(display) = gtk::gdk::Display::default() {
            let icon_theme = gtk::IconTheme::for_display(&display);
            icon_theme.add_resource_path("/com/cali666/lsfg-vk-ui/icons");
        }
    });

    application.connect_activate(move |app| {
        // Load initial configuration
        let initial_config = load_config().unwrap_or_else(|e| {
            eprintln!("Error loading config: {}", e);
            // Corrected Config initialization
            config::Config { version: 1, ordered_global: OrderedGlobalConfig { global: None }, game: Vec::new() }
        });

        // Load UI from .ui file
        let ui_bytes = include_bytes!("../resources/ui.ui");
        let builder = Builder::from_string(std::str::from_utf8(ui_bytes).unwrap());

        // Get main window and other widgets
        let main_window: ApplicationWindow = builder
            .object("main_window")
            .expect("Could not get main_window from builder");
        main_window.set_application(Some(app));

        let settings_button: gtk::Button = builder
            .object("settings_button")
            .expect("Could not get settings_button from builder");

        // Set application icon for proper dock integration
        main_window.set_icon_name(Some("com.cali666.lsfg-vk-ui"));

        let sidebar_list_box: gtk::ListBox = builder
            .object("sidebar_list_box")
            .expect("Could not get sidebar_list_box from builder");
        let create_profile_button: gtk::Button = builder
            .object("create_profile_button")
            .expect("Could not get create_profile_button from builder");

        let multiplier_dropdown: gtk::DropDown = builder
            .object("multiplier_dropdown")
            .expect("Could not get multiplier_dropdown from builder");
        let flow_scale_entry: gtk::Entry = builder
            .object("flow_scale_entry")
            .expect("Could not get flow_scale_entry from builder");
        let performance_mode_switch: gtk::Switch = builder
            .object("performance_mode_switch")
            .expect("Could not get performance_mode_switch from builder");
        let hdr_mode_switch: gtk::Switch = builder
            .object("hdr_mode_switch")
            .expect("Could not get hdr_mode_switch from builder");
        let experimental_present_mode_dropdown: gtk::DropDown = builder
            .object("experimental_present_mode_dropdown")
            .expect("Could not get experimental_present_mode_dropdown from builder");

        let main_stack: gtk::Stack = builder
            .object("main_stack")
            .expect("Could not get main_stack from builder. Ensure it has id='main_stack' in ui.ui.");
        let main_stack_switcher: gtk::StackSwitcher = builder
            .object("main_stack_switcher")
            .expect("Could not get main_stack_switcher from builder. Ensure it has id='main_stack_switcher' in ui.ui.");
        
        main_stack_switcher.set_stack(Some(&main_stack));

        let main_settings_box: gtk::Box = builder
            .object("main_box")
            .expect("Could not get main_box from builder");

        let save_button = gtk::Button::builder()
            .label("Save Changes")
            .halign(gtk::Align::End)
            .margin_end(12)
            .margin_bottom(12)
            .build();

        main_settings_box.append(&save_button);

        // Initialize application state (with None for handler IDs initially)
        let app_state = Rc::new(RefCell::new(AppState {
            config: initial_config,
            selected_profile_index: None,
            main_window: main_window.clone(),
            sidebar_list_box: sidebar_list_box.clone(),
            multiplier_dropdown: multiplier_dropdown.clone(),
            flow_scale_entry: flow_scale_entry.clone(),
            performance_mode_switch: performance_mode_switch.clone(),
            hdr_mode_switch: hdr_mode_switch.clone(),
            experimental_present_mode_dropdown: experimental_present_mode_dropdown.clone(),
            save_button: save_button.clone(),
            main_settings_box: main_settings_box.clone(),
            main_stack: main_stack.clone(),
            multiplier_dropdown_handler_id: None,
            flow_scale_entry_handler_id: None,
            performance_mode_switch_handler_id: None,
            hdr_mode_switch_handler_id: None,
            experimental_present_mode_dropdown_handler_id: None,
        }));

        // --- Connect Signals ---

        // Connect settings button
        let main_window_clone = main_window.clone();
        let app_state_clone_for_settings = app_state.clone(); // Clone for settings window
        settings_button.connect_clicked(move |_| {
            let settings_win = settings_window::create_settings_window(&main_window_clone, app_state_clone_for_settings.clone());
            settings_win.present();
        });

        let app_state_clone = app_state.clone();
        sidebar_list_box.connect_row_activated(move |_list_box, row| {
            let index = row.index() as usize;
            let mut state = app_state_clone.borrow_mut();
            state.selected_profile_index = Some(index);
            drop(state);

            let app_state_for_idle = app_state_clone.clone();
            glib::idle_add_local(move || {
                app_state_for_idle.borrow().update_main_window_from_profile();
                glib::ControlFlow::Break
            });
        });

        let app_state_clone = app_state.clone();
        create_profile_button.connect_clicked(move |_| {
            let dialog = gtk::MessageDialog::new(
                Some(&app_state_clone.borrow().main_window),
                gtk::DialogFlags::MODAL,
                gtk::MessageType::Question,
                gtk::ButtonsType::None,
                "",
            );
            dialog.set_title(Some("New Profile"));
            dialog.set_secondary_text(Some("Enter or browse Application Name"));

            let entry = gtk::Entry::builder()
                .placeholder_text("Application Name")
                .hexpand(true)
                .build();
            
            let pick_process_button = gtk::Button::builder()
                .label("🖵")
                .tooltip_text("Pick a running Vulkan process")
                .css_classes(["flat", "square", "icon-button"])
                .build();

            let entry_box = gtk::Box::builder()
                .orientation(gtk::Orientation::Horizontal)
                .spacing(6)
                .margin_top(12)
                .margin_bottom(12)
                .margin_start(12)
                .margin_end(12)
                .build();
            entry_box.append(&entry);
            entry_box.append(&pick_process_button);

            dialog.content_area().append(&entry_box);

            dialog.add_button("Cancel", gtk::ResponseType::Cancel);
            dialog.add_button("Create", gtk::ResponseType::Other(1));

            dialog.set_default_response(gtk::ResponseType::Other(1));

            // Allow pressing Enter in the entry to trigger the "Create" button
            let dialog_clone = dialog.clone();
            entry.connect_activate(move |_| {
                dialog_clone.response(gtk::ResponseType::Other(1));
            });

            // --- Process Picker Button Logic ---
            let entry_clone_for_picker = entry.clone();
            let main_window_clone_for_picker = app_state_clone.borrow().main_window.clone();

            pick_process_button.connect_clicked(move |_| {
                let process_picker_window = libadwaita::ApplicationWindow::builder()
                    .title("Select Process")
                    .transient_for(&main_window_clone_for_picker)
                    .modal(true)
                    .default_width(400)
                    .default_height(600)
                    .build();

                let scrolled_window = gtk::ScrolledWindow::builder()
                    .hscrollbar_policy(gtk::PolicyType::Never)
                    .vscrollbar_policy(gtk::PolicyType::Automatic)
                    .hexpand(true) // Make the scrolled window expand horizontally
                    .vexpand(true) // Make the scrolled window expand vertically
                    .margin_top(12)
                    .margin_start(12)
                    .margin_end(12)
                    .build();
                
                let process_list_box = gtk::ListBox::builder()
                    .selection_mode(gtk::SelectionMode::Single)
                    .build();
                scrolled_window.set_child(Some(&process_list_box));

                let content_box = gtk::Box::builder()
                    .orientation(gtk::Orientation::Vertical)
                    .build();
                content_box.append(&scrolled_window); // Add scrolled window first to take up space

                let close_button = gtk::Button::builder()
                    .label("Close")
                    .halign(gtk::Align::End)
                    .margin_end(12)
                    .margin_bottom(12)
                    .build();
                content_box.append(&close_button); // Add close button at the bottom

                process_picker_window.set_content(Some(&content_box));

                // Populate the list with processes
                let processes = utils::get_vulkan_processes(); // Call the new function from utils.rs
                for proc_name in processes {
                    let row = gtk::ListBoxRow::new();
                    let label = gtk::Label::builder()
                        .label(&proc_name)
                        .halign(gtk::Align::Start)
                        .margin_start(12)
                        .margin_end(12)
                        .margin_top(8)
                        .margin_bottom(8)
                        .build();
                    row.set_child(Some(&label));
                    process_list_box.append(&row);
                }

                // Connect selection handler
                let entry_clone_for_select = entry_clone_for_picker.clone();
                let picker_window_clone = process_picker_window.clone();
                process_list_box.connect_row_activated(move |_list_box, row| {
                    if let Some(label_widget) = row.child().and_then(|c| c.downcast::<gtk::Label>().ok()) {
                        let process_name = label_widget.label().to_string();
                        entry_clone_for_select.set_text(&process_name);
                        picker_window_clone.close();
                    }
                });

                // Connect close button
                let picker_window_clone_for_close = process_picker_window.clone();
                close_button.connect_clicked(move |_| {
                    picker_window_clone_for_close.close();
                });

                process_picker_window.present();
            });
            // --- End Process Picker Button Logic ---

            let app_state_clone_dialog = app_state_clone.clone();
            let entry_clone = entry.clone();
            dialog.connect_response(
                move |d: &gtk::MessageDialog, response: gtk::ResponseType| {
                    if response == gtk::ResponseType::Other(1) {
                        let game_name = entry_clone.text().to_string();
                        if !game_name.is_empty() {
                            let mut state = app_state_clone_dialog.borrow_mut();
                            
                            if state.config.game.iter().any(|p| p.exe == game_name) {
                                let error_dialog = gtk::MessageDialog::new(
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

                            let new_profile = config::GameProfile {
                                exe: game_name,
                                ..Default::default()
                            };
                            
                            state.config.game.push(new_profile);
                            state.selected_profile_index = Some(state.config.game.len() - 1);
                            
                            state.save_current_config();
                            
                            state.populate_sidebar_with_handlers(Some(app_state_clone_dialog.clone()));
                            drop(state);

                            let app_state_for_idle = app_state_clone_dialog.clone();
                            glib::idle_add_local(move || {
                                app_state_for_idle.borrow().update_main_window_from_profile();
                                glib::ControlFlow::Break
                            });
                        }
                    }
                    d.close();
                }
            );
            dialog.present();
        });

        let app_state_clone_for_handler_mult = app_state.clone();
        let multiplier_handler_id = multiplier_dropdown.connect_selected_item_notify(move |dropdown| {
            let mut state = app_state_clone_for_handler_mult.borrow_mut();
            
            if let Some(index) = state.selected_profile_index {
                if index < state.config.game.len() {
                    if let Some(profile) = state.config.game.get_mut(index) {
                        if let Some(item) = dropdown.selected_item() {
                            if let Some(string_obj) = item.downcast_ref::<gtk::StringObject>() {
                                let text = string_obj.string();
                                profile.multiplier = match text.as_str() {
                                    "off" => 1,
                                    _ => text.parse().unwrap_or(1),
                                };
                            }
                        }
                    }
                }
            }
        });
        app_state.borrow_mut().multiplier_dropdown_handler_id = Some(multiplier_handler_id);

        let app_state_clone_for_handler_flow = app_state.clone();
        let flow_handler_id = flow_scale_entry.connect_changed(move |entry| {
            let mut state = app_state_clone_for_handler_flow.borrow_mut();
            if let Some(index) = state.selected_profile_index {
                if let Some(profile) = state.config.game.get_mut(index) {
                    if let Ok(value) = entry.text().parse::<f32>() {
                        profile.flow_scale = round_to_2_decimals(value);
                    }
                }
            }
        });
        app_state.borrow_mut().flow_scale_entry_handler_id = Some(flow_handler_id);

        let app_state_clone_for_handler_perf = app_state.clone();
        let perf_handler_id = performance_mode_switch.connect_state_set(move |_sw, active| {
            let mut state = app_state_clone_for_handler_perf.borrow_mut();
            if let Some(index) = state.selected_profile_index {
                if let Some(profile) = state.config.game.get_mut(index) {
                    profile.performance_mode = active;
                }
            }
            drop(state);
            glib::Propagation::Proceed
        });
        app_state.borrow_mut().performance_mode_switch_handler_id = Some(perf_handler_id);

        let app_state_clone_for_handler_hdr = app_state.clone();
        let hdr_handler_id = hdr_mode_switch.connect_state_set(move |_sw, active| {
            let mut state = app_state_clone_for_handler_hdr.borrow_mut();
            if let Some(index) = state.selected_profile_index {
                if let Some(profile) = state.config.game.get_mut(index) {
                    profile.hdr_mode = active;
                }
            }
            drop(state);
            glib::Propagation::Proceed
        });
        app_state.borrow_mut().hdr_mode_switch_handler_id = Some(hdr_handler_id);

        let app_state_clone_for_handler_exp = app_state.clone();
        let exp_handler_id = experimental_present_mode_dropdown.connect_selected_item_notify(move |dropdown| {
            let mut state = app_state_clone_for_handler_exp.borrow_mut();
            if let Some(index) = state.selected_profile_index {
                if let Some(profile) = state.config.game.get_mut(index) {
                    let selected_text = dropdown.selected_item().and_then(|item| item.downcast_ref::<gtk::StringObject>().map(|s| s.string().to_string()));
                    if let Some(text) = selected_text {
                        profile.experimental_present_mode = text;
                    }
                }
            }
        });
        app_state.borrow_mut().experimental_present_mode_dropdown_handler_id = Some(exp_handler_id);

        let app_state_clone_save = app_state.clone();
        save_button.connect_clicked(move |_| {
            let state_ref = app_state_clone_save.borrow();
            if let Some(index) = state_ref.selected_profile_index {
                let multiplier_str = state_ref.multiplier_dropdown.selected_item().and_then(|item| item.downcast_ref::<gtk::StringObject>().map(|s| s.string().to_string()));
                let flow_scale_text = state_ref.flow_scale_entry.text().to_string();
                let performance_mode_active = state_ref.performance_mode_switch.is_active();
                let hdr_mode_active = state_ref.hdr_mode_switch.is_active();
                let exp_mode_str = state_ref.experimental_present_mode_dropdown.selected_item().and_then(|item| item.downcast_ref::<gtk::StringObject>().map(|s| s.string().to_string()));
                
                drop(state_ref);

                let mut state = app_state_clone_save.borrow_mut();
                if let Some(profile) = state.config.game.get_mut(index) {
                    if let Some(text) = multiplier_str {
                        profile.multiplier = if text == "off" { 1 } else { text.parse().unwrap_or(1) };
                    }

                    if let Ok(value) = flow_scale_text.parse::<f32>() {
                        profile.flow_scale = round_to_2_decimals(value);
                    }

                    profile.performance_mode = performance_mode_active;
                    profile.hdr_mode = hdr_mode_active;

                    if let Some(text) = exp_mode_str {
                        profile.experimental_present_mode = text;
                    }

                    state.save_current_config();

                    let feedback_label = Label::new(Some("Saved!"));
                    feedback_label.set_halign(gtk::Align::End);
                    feedback_label.set_margin_end(12);
                    feedback_label.set_margin_bottom(12);
                    
                    let main_settings_box_clone = state.main_settings_box.clone();
                    
                    main_settings_box_clone.append(&feedback_label);

                    glib::timeout_add_local(std::time::Duration::new(2, 0), move || {
                        main_settings_box_clone.remove(&feedback_label);
                        glib::ControlFlow::Break
                    });
                }
            }
        });

        let app_state_clone_initial = app_state.clone();
        glib::idle_add_local(move || {
            let mut state = app_state_clone_initial.borrow_mut();
            if state.config.game.first().is_some() {
                state.selected_profile_index = Some(0);
            }
            state.populate_sidebar_with_handlers(Some(app_state_clone_initial.clone()));
            drop(state);
            
            if app_state_clone_initial.borrow().selected_profile_index.is_some() {
                app_state_clone_initial.borrow().update_main_window_from_profile();
            }
            glib::ControlFlow::Break
        });

        main_window.present();
    });

    application.run()
}

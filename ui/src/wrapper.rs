use gtk::glib;
use gtk;
use adw;
use gtk::glib::types::StaticTypeExt;

pub mod pane;
pub mod pref;

pub mod imp {
    use gtk::subclass::prelude::*;
    use adw::subclass::prelude::*;
    use crate::wrapper::pane::*;
    use gtk::{glib, CompositeTemplate};

    #[derive(CompositeTemplate, Default)]
    #[template(resource = "/gay/pancake/lsfg-vk/window.ui")]
    pub struct Window {
        #[template_child]
        pub main: TemplateChild<PaneMain>,
        #[template_child]
        pub sidebar: TemplateChild<PaneSidebar>,
    }

    #[glib::object_subclass]
    impl ObjectSubclass for Window {
        const NAME: &'static str = "LSApplicationWindow";
        type Type = super::Window;
        type ParentType = adw::ApplicationWindow;

        fn class_init(klass: &mut Self::Class) {
            klass.bind_template();
        }

        fn instance_init(obj: &glib::subclass::InitializingObject<Self>) {
            obj.init_template();
        }
    }

    impl ObjectImpl for Window {
        fn constructed(&self) {
            self.parent_constructed();
        }
    }

    impl WidgetImpl for Window {}
    impl WindowImpl for Window {}
    impl ApplicationWindowImpl for Window {}
    impl AdwWindowImpl for Window {}
    impl AdwApplicationWindowImpl for Window {}
}

glib::wrapper! {
    pub struct Window(ObjectSubclass<imp::Window>)
        @extends
            adw::ApplicationWindow, adw::Window,
            gtk::ApplicationWindow, gtk::Window, gtk::Widget,
        @implements
            gtk::gio::ActionGroup, gtk::gio::ActionMap,
            gtk::Accessible, gtk::Buildable, gtk::ConstraintTarget,
            gtk::Native, gtk::Root, gtk::ShortcutManager;
}

impl Window {
    pub fn new(app: &adw::Application) -> Self {
        pref::PrefDropdown::ensure_type();
        pref::PrefEntry::ensure_type();
        pref::PrefNumber::ensure_type();
        pref::PrefSlider::ensure_type();
        pref::PrefSwitch::ensure_type();
        pane::PaneMain::ensure_type();
        pane::PaneSidebar::ensure_type();

        glib::Object::builder()
            .property("application", app)
            .build()
    }
}

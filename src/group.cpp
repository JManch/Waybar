#include "group.hpp"

#include <fmt/format.h>

#include <util/command.hpp>

#include "gdkmm/device.h"
#include "gtkmm/widget.h"

namespace waybar {

const Gtk::RevealerTransitionType getPreferredTransitionType(bool is_vertical, bool left_to_right) {
  if (is_vertical) {
    if (left_to_right) {
      return Gtk::RevealerTransitionType::REVEALER_TRANSITION_TYPE_SLIDE_DOWN;
    } else {
      return Gtk::RevealerTransitionType::REVEALER_TRANSITION_TYPE_SLIDE_UP;
    }
  } else {
    if (left_to_right) {
      return Gtk::RevealerTransitionType::REVEALER_TRANSITION_TYPE_SLIDE_RIGHT;
    } else {
      return Gtk::RevealerTransitionType::REVEALER_TRANSITION_TYPE_SLIDE_LEFT;
    }
  }
}

Group::Group(const std::string& name, const std::string& id, const Json::Value& config,
             bool vertical)
    : AModule(config, name, id, true, true),
      box{vertical ? Gtk::ORIENTATION_VERTICAL : Gtk::ORIENTATION_HORIZONTAL, 0},
      revealer_box{vertical ? Gtk::ORIENTATION_VERTICAL : Gtk::ORIENTATION_HORIZONTAL, 0} {
  box.set_name(name_);
  if (!id.empty()) {
    box.get_style_context()->add_class(id);
  }

  // default orientation: orthogonal to parent
  auto orientation =
      config_["orientation"].empty() ? "orthogonal" : config_["orientation"].asString();
  if (orientation == "inherit") {
    // keep orientation passed
  } else if (orientation == "orthogonal") {
    box.set_orientation(vertical ? Gtk::ORIENTATION_HORIZONTAL : Gtk::ORIENTATION_VERTICAL);
  } else if (orientation == "vertical") {
    box.set_orientation(Gtk::ORIENTATION_VERTICAL);
  } else if (orientation == "horizontal") {
    box.set_orientation(Gtk::ORIENTATION_HORIZONTAL);
  } else {
    throw std::runtime_error("Invalid orientation value: " + orientation);
  }

  if (config_["drawer"].isObject()) {
    is_drawer = true;

    const auto& drawer_config = config_["drawer"];
    const int transition_duration =
        (drawer_config["transition-duration"].isInt() ? drawer_config["transition-duration"].asInt()
                                                      : 500);
    add_class_to_drawer_children =
        (drawer_config["children-class"].isString() ? drawer_config["children-class"].asString()
                                                    : "drawer-child");
    const bool left_to_right = (drawer_config["transition-left-to-right"].isBool()
                                    ? drawer_config["transition-left-to-right"].asBool()
                                    : true);

    auto transition_type = getPreferredTransitionType(vertical, left_to_right);

    revealer.set_transition_type(transition_type);
    revealer.set_transition_duration(transition_duration);
    revealer.set_reveal_child(false);

    revealer.get_style_context()->add_class("drawer");

    revealer.add(revealer_box);
    box.pack_start(revealer);

    addHoverHandlerTo(revealer);
  }
}

bool Group::handleMouseHover(GdkEventCrossing* const& e) {
  switch (e->type) {
    case GDK_ENTER_NOTIFY:
      revealer.set_reveal_child(true);
      break;
    case GDK_LEAVE_NOTIFY:
      revealer.set_reveal_child(false);
      break;
    default:
      break;
  }

  return true;
}

void Group::addHoverHandlerTo(Gtk::Widget& widget) {
  widget.add_events(Gdk::EventMask::ENTER_NOTIFY_MASK | Gdk::EventMask::LEAVE_NOTIFY_MASK);
  widget.signal_enter_notify_event().connect(sigc::mem_fun(*this, &Group::handleMouseHover));
  widget.signal_leave_notify_event().connect(sigc::mem_fun(*this, &Group::handleMouseHover));
}

auto Group::update() -> void {
  // noop
}

Gtk::Box& Group::getBox() { return is_drawer ? (is_first_widget ? box : revealer_box) : box; }

void Group::addWidget(Gtk::Widget& widget) {
  if (is_drawer) {
    getBox().pack_start(widget, false, false);

    if (is_first_widget) {
      // Necessary because of GTK's hitbox detection
      addHoverHandlerTo(widget);
    } else {
      widget.get_style_context()->add_class(add_class_to_drawer_children);
    }
  } else {
    getBox().pack_start(widget, false, false);
  }

  is_first_widget = false;
}

Group::operator Gtk::Widget&() { return box; }

}  // namespace waybar

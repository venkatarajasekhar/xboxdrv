/* 
**  Xbox/Xbox360 USB Gamepad Userspace Driver
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "xboxmsg.hpp"

#include <boost/format.hpp>

#include "helper.hpp"
#include "raise_exception.hpp"

int16_t u8_to_s16(uint8_t value)
{
  // FIXME: verify this
  if (value < 128)
  {
    return -32768 + (value * 32768 / 128);
  }
  else
  {
    return (value-128) * 32767 / 127;
  }
}

float s16_to_float(int16_t value)
{
  if (value >= 0)
  {
    return static_cast<float>(value) / 32767.0f;
  }
  else
  {
    return static_cast<float>(value) / 32768.0f;
  }
}

/**
   input:  [0, 255]
   output: [ -1.0f, 1.0f ] 
*/
float u8_to_float(uint8_t value)
{
  return static_cast<float>(value) / 255.0f * 2.0f - 1.0f;
}

int16_t float_to_s16(float v)
{
  if (v >= 0.0f)
  {
    return static_cast<int16_t>(std::min(1.0f, v) * 32767.0f);
  }
  else
  {
    return static_cast<int16_t>(std::max(-1.0f, v) * 32768.0f);
  }
}

/**
   input:  [ -1.0f, 1.0f ] 
   output: [0, 255]
*/
uint8_t float_to_u8(float v)
{
  return static_cast<uint8_t>(Math::clamp(0.0f, (v + 1.0f) / 2.0f, 1.0f) * 255.0f);
}

XboxGenericMsg::XboxGenericMsg() :
  m_axis_state(),
  m_button_state()
{
  clear();
}

void
XboxGenericMsg::clear()
{
  std::fill_n(m_axis_state,   static_cast<int>(XBOX_AXIS_MAX), 0);
  std::fill_n(m_button_state, static_cast<int>(XBOX_BTN_MAX),  false);

  std::fill_n(m_axis_set,   static_cast<int>(XBOX_AXIS_MAX), false);
  std::fill_n(m_button_set, static_cast<int>(XBOX_BTN_MAX),  false);
}

bool
XboxGenericMsg::get_button(XboxButton button) const
{
  if (m_button_set[button])
  {
    return m_button_state[button];
  }
  else
  {
    switch(button)
    {
      case XBOX_BTN_A:
        return m_axis_state[XBOX_AXIS_A];
      case XBOX_BTN_B:
        return m_axis_state[XBOX_AXIS_B];
      case XBOX_BTN_X:
        return m_axis_state[XBOX_AXIS_X];
      case XBOX_BTN_Y:
        return m_axis_state[XBOX_AXIS_Y];

      case XBOX_BTN_LB:
        return m_axis_state[XBOX_AXIS_BLACK];
      case XBOX_BTN_RB:
        return m_axis_state[XBOX_AXIS_WHITE];

      case XBOX_BTN_LT:
        return m_axis_state[XBOX_AXIS_LT];
      case XBOX_BTN_RT:
        return m_axis_state[XBOX_AXIS_RT];

      case XBOX_DPAD_UP:
        return m_axis_state[XBOX_AXIS_DPAD_Y] < 0;
      case XBOX_DPAD_DOWN:
        return m_axis_state[XBOX_AXIS_DPAD_Y] > 0;       
      case XBOX_DPAD_LEFT:
        return m_axis_state[XBOX_AXIS_DPAD_X] < 0;
      case XBOX_DPAD_RIGHT:
        return m_axis_state[XBOX_AXIS_DPAD_X] > 0;

      default:
        return false;
    }
  }
}

void
XboxGenericMsg::set_button(XboxButton button, bool v)
{
  m_button_set[button] = true;
  m_button_state[button] = v;
}

int
XboxGenericMsg::get_axis(XboxAxis axis) const
{
  if (m_axis_set[axis])
  {
    return m_axis_state[axis];
  }
  else
  {
    switch(axis)
    {
      case XBOX_AXIS_A:
        return m_button_state[XBOX_BTN_A];

      case XBOX_AXIS_B:
        return m_button_state[XBOX_BTN_B];

      case XBOX_AXIS_X:
        return m_button_state[XBOX_BTN_X];

      case XBOX_AXIS_Y:
        return m_button_state[XBOX_BTN_Y];

      case XBOX_AXIS_LT:
        return m_button_state[XBOX_BTN_LT] * 255;

      case XBOX_AXIS_RT:
        return m_button_state[XBOX_BTN_RT] * 255;

      case XBOX_AXIS_BLACK:
        return m_button_state[XBOX_BTN_LB] * 255;

      case XBOX_AXIS_WHITE:
        return m_button_state[XBOX_BTN_RB] * 255;

      case XBOX_AXIS_DPAD_X:
        if (m_button_state[XBOX_DPAD_LEFT] && !m_button_state[XBOX_DPAD_RIGHT])
          return -1;
        else if (!m_button_state[XBOX_DPAD_LEFT] && m_button_state[XBOX_DPAD_RIGHT])
          return 1;
        else
          return 0;

      case XBOX_AXIS_DPAD_Y:
        if (m_button_state[XBOX_DPAD_UP] && !m_button_state[XBOX_DPAD_DOWN])
          return -1;
        else if (!m_button_state[XBOX_DPAD_UP] && m_button_state[XBOX_DPAD_DOWN])
          return 1;
        else
          return 0;

      case XBOX_AXIS_TRIGGER:
        return -get_axis(XBOX_AXIS_LT) + get_axis(XBOX_AXIS_RT);

      default:
        return 0;
    }
  }
}

void
XboxGenericMsg::set_axis(XboxAxis axis, int v)
{
  m_axis_set[axis] = true;
  m_axis_state[axis] = v;
}

float
XboxGenericMsg::get_axis_float(XboxAxis axis) const
{
  return to_float(m_axis_state[axis], get_axis_min(axis), get_axis_max(axis));
}

void
XboxGenericMsg::set_axis_float(XboxAxis axis, float v)
{
  m_axis_state[axis] = from_float(v, get_axis_min(axis), get_axis_max(axis));
}

std::string gamepadtype_to_string(const GamepadType& type)
{
  switch (type)
  {
    case GAMEPAD_XBOX360:
      return "xbox360";

    case GAMEPAD_XBOX360_WIRELESS:
      return "xbox360-wireless";

    case GAMEPAD_XBOX360_PLAY_N_CHARGE: 
      return "xbox360-playncharge";

    case GAMEPAD_XBOX:
      return "xbox";

    case GAMEPAD_XBOX_MAT:
      return "xbox-mat";
        
    case GAMEPAD_XBOX360_GUITAR:
      return "xbox360-guitar";

    case GAMEPAD_FIRESTORM:
      return "firestorm";

    case GAMEPAD_FIRESTORM_VSB:
      return "firestorm-vsb";

    case GAMEPAD_SAITEK_P2500:
      return "saitek-p2500";

    case GAMEPAD_PLAYSTATION3_USB:
      return "playstation3-usb";

    default:
      assert(!"Unknown gamepad type supplied");
  }
}


std::string gamepadtype_to_macro_string(const GamepadType& type)
{
  switch (type)
  {
    case GAMEPAD_XBOX360: return "GAMEPAD_XBOX360";
    case GAMEPAD_XBOX360_WIRELESS: return "GAMEPAD_XBOX360_WIRELESS";
    case GAMEPAD_XBOX360_PLAY_N_CHARGE: return "GAMEPAD_XBOX360_PLAY_N_CHARGE";
    case GAMEPAD_XBOX: return "GAMEPAD_XBOX";
    case GAMEPAD_XBOX_MAT: return "GAMEPAD_XBOX_MAT";
    case GAMEPAD_XBOX360_GUITAR: return "GAMEPAD_XBOX360_GUITAR";
    case GAMEPAD_FIRESTORM: return "GAMEPAD_FIRESTORM";
    case GAMEPAD_FIRESTORM_VSB: return "GAMEPAD_FIRESTORM_VSB";
    case GAMEPAD_SAITEK_P2500: return "GAMEPAD_SAITEK_P2500";
    case GAMEPAD_PLAYSTATION3_USB: return "GAMEPAD_PLAYSTATION3_USB";
    default:
      assert(!"Unknown gamepad type supplied");
  }
}

std::ostream& operator<<(std::ostream& out, const GamepadType& type) 
{
  switch (type)
  {
    case GAMEPAD_XBOX360:
      return out << "Xbox360";

    case GAMEPAD_XBOX360_WIRELESS:
      return out << "Xbox360 (wireless)";

    case GAMEPAD_XBOX360_PLAY_N_CHARGE: 
      return out << "Xbox360 Play&Charge";

    case GAMEPAD_XBOX:
      return out << "Xbox Classic";

    case GAMEPAD_XBOX_MAT:
      return out << "Xbox Dancepad";
        
    case GAMEPAD_XBOX360_GUITAR:
      return out << "Xbox360 Guitar";

    case GAMEPAD_FIRESTORM:
      return out << "Firestorm Dual Power";

    case GAMEPAD_FIRESTORM_VSB:
      return out << "Firestorm Dual Power (vsb)";

    case GAMEPAD_SAITEK_P2500:
      return out << "Saitek P2500";

    case GAMEPAD_PLAYSTATION3_USB:
      return out << "Playstation 3 USB";

    default:
      return out << "unknown" << std::endl;
  }
}

std::ostream& operator<<(std::ostream& out, const XboxGenericMsg& msg)
{
  for(int i = 1; i < XBOX_AXIS_MAX; ++i)
  {
    XboxAxis axis = static_cast<XboxAxis>(i);

    out << axis2string(axis) << ":" << msg.get_axis(static_cast<XboxAxis>(axis)) << " ";
  }

  for(int i = 1; i < XBOX_BTN_MAX; ++i)
  {
    XboxButton btn = static_cast<XboxButton>(i);

    out << btn2string(btn) << ":" << msg.get_button(btn) << " ";
  }
  
  return out;
}

XboxButton string2btn(const std::string& str_)
{
  std::string str = to_lower(str_);

  if (str == "start")
    return XBOX_BTN_START;
  else if (str == "guide" || str == "ps")
    return XBOX_BTN_GUIDE;
  else if (str == "back" || str == "select")
    return XBOX_BTN_BACK;

  else if (str == "a" || str == "1" || str == "green" || str == "cross")
    return XBOX_BTN_A;
  else if (str == "b" || str == "2" || str == "red" || str == "circle")
    return XBOX_BTN_B;
  else if (str == "x" || str == "3" || str == "blue" || str == "square")
    return XBOX_BTN_X;
  else if (str == "y" || str == "4" || str == "yellow" || str == "triangle")
    return XBOX_BTN_Y;

  else if (str == "lb" || str == "5" || str == "orange" || str == "white" || str == "l1")
    return XBOX_BTN_LB;
  else if (str == "rb" || str == "6" || str == "black" || str == "r1")
    return XBOX_BTN_RB;

  else if (str == "lt" || str == "7" || str == "l2")
    return XBOX_BTN_LT;
  else if (str == "rt" || str == "8" || str == "r2")
    return XBOX_BTN_RT;

  else if (str == "tl" || str == "l3")
    return XBOX_BTN_THUMB_L;
  else if (str == "tr" || str == "r3")
    return XBOX_BTN_THUMB_R;

  else if (str == "du" || str == "up")
    return XBOX_DPAD_UP;
  else if (str == "dd" || str == "down")
    return XBOX_DPAD_DOWN;
  else if (str == "dl" || str == "left")
    return XBOX_DPAD_LEFT;
  else if (str == "dr" || str == "right")
    return XBOX_DPAD_RIGHT;

  else
    raise_exception(std::runtime_error, "couldn't convert string \"" + str + "\" to XboxButton");
}

XboxAxis string2axis(const std::string& str_)
{
  std::string str = to_lower(str_);
  if (str == "x1")
    return XBOX_AXIS_X1;
  else if (str == "y1")
    return XBOX_AXIS_Y1;
  
  else if (str == "x2" || str == "whammy")
    return XBOX_AXIS_X2;
  else if (str == "y2" || str == "tilt")
    return XBOX_AXIS_Y2;
  
  else if (str == "lt" || str == "l2")
    return XBOX_AXIS_LT;
  else if (str == "rt" || str == "r2")
    return XBOX_AXIS_RT;

  else if (str == "dpad_x")
    return XBOX_AXIS_DPAD_X;
  else if (str == "dpad_y")
    return XBOX_AXIS_DPAD_Y;

  else if (str == "trigger" || str == "z" || str == "rudder")
    return XBOX_AXIS_TRIGGER;

  else if (str == "btn_a" || str == "cross")
    return XBOX_AXIS_A;

  else if (str == "btn_b" || str == "circle")
    return XBOX_AXIS_B;

  else if (str == "btn_x" || str == "square")
    return XBOX_AXIS_X;

  else if (str == "btn_y" || str == "triangle")
    return XBOX_AXIS_Y;

  else if (str == "white" || str == "lb"|| str == "l1")
    return XBOX_AXIS_WHITE;

  else if (str == "black" || str == "rb" || str == "r1")
    return XBOX_AXIS_BLACK;

  else
    raise_exception(std::runtime_error, "couldn't convert string \"" + str + "\" to XboxAxis");
}

std::string axis2string(XboxAxis axis)
{
  switch(axis)
  {
    case XBOX_AXIS_MAX:
    case XBOX_AXIS_UNKNOWN: return "unknown";

    case XBOX_AXIS_TRIGGER: return "TRIGGER";

    case XBOX_AXIS_DPAD_X: return "DPAD_X";
    case XBOX_AXIS_DPAD_Y: return "DPAD_Y";

    case XBOX_AXIS_X1: return "X1";
    case XBOX_AXIS_Y1: return "Y1";

    case XBOX_AXIS_X2: return "X2";
    case XBOX_AXIS_Y2: return "Y2";

    case XBOX_AXIS_LT: return "LT";
    case XBOX_AXIS_RT: return "RT";

    case XBOX_AXIS_A:     return "BTN_A";
    case XBOX_AXIS_B:     return "BTN_B";
    case XBOX_AXIS_X:     return "BTN_X"; 
    case XBOX_AXIS_Y:     return "BTN_Y"; 
    case XBOX_AXIS_BLACK: return "Black";
    case XBOX_AXIS_WHITE: return "White";
  }
  return "unknown";
}

std::string btn2string(XboxButton btn)
{
  switch (btn)
  {
    case XBOX_BTN_MAX:
    case XBOX_BTN_UNKNOWN: return "unknown";

    case XBOX_BTN_START: return "Start";
    case XBOX_BTN_GUIDE: return "Guide";
    case XBOX_BTN_BACK: return "Back";

    case XBOX_BTN_A: return "A";
    case XBOX_BTN_B: return "B";
    case XBOX_BTN_X: return "X";
    case XBOX_BTN_Y: return "Y";

    case XBOX_BTN_LB: return "LB";
    case XBOX_BTN_RB: return "RB";

    case XBOX_BTN_LT: return "LT";
    case XBOX_BTN_RT: return "RT";

    case XBOX_BTN_THUMB_L: return "TL";
    case XBOX_BTN_THUMB_R: return "TR";

    case XBOX_DPAD_UP:    return "DPAD_UP";
    case XBOX_DPAD_DOWN:  return "DPAD_DOWN";
    case XBOX_DPAD_LEFT:  return "DPAD_LEFT";
    case XBOX_DPAD_RIGHT: return "DPAD_RIGHT";
  }
  return "unknown";
}

int
XboxGenericMsg::get_axis_min(XboxAxis axis)
{
  switch(axis)
  {
    case XBOX_AXIS_X1: return -32768;
    case XBOX_AXIS_Y1: return -32768;
    
    case XBOX_AXIS_X2: return -32768;
    case XBOX_AXIS_Y2: return -32768;

    case XBOX_AXIS_LT: return 0;
    case XBOX_AXIS_RT: return 0;

    case XBOX_AXIS_DPAD_X: return -1;
    case XBOX_AXIS_DPAD_Y: return -1;

    case XBOX_AXIS_TRIGGER: return -255;

    case XBOX_AXIS_A:     return 0;
    case XBOX_AXIS_B:     return 0;
    case XBOX_AXIS_X:     return 0;
    case XBOX_AXIS_Y:     return 0;
    case XBOX_AXIS_BLACK: return 0;
    case XBOX_AXIS_WHITE: return 0;

    default: assert(!"never reached");
  }
}

int
XboxGenericMsg::get_axis_max(XboxAxis axis)
{
  switch(axis)
  {
    case XBOX_AXIS_X1: return 32767;
    case XBOX_AXIS_Y1: return 32767;
    
    case XBOX_AXIS_X2: return 32767;
    case XBOX_AXIS_Y2: return 32767;

    case XBOX_AXIS_LT: return 255;
    case XBOX_AXIS_RT: return 255;

    case XBOX_AXIS_DPAD_X: return 1;
    case XBOX_AXIS_DPAD_Y: return 1;

    case XBOX_AXIS_TRIGGER: return 255;

    case XBOX_AXIS_A:     return 255;
    case XBOX_AXIS_B:     return 255;
    case XBOX_AXIS_X:     return 255;
    case XBOX_AXIS_Y:     return 255;
    case XBOX_AXIS_BLACK: return 255;
    case XBOX_AXIS_WHITE: return 255;

    default: assert(!"never reached");
  }
}

void
XboxGenericMsg::set_axis_min(XboxAxis axis, int value)
{
}

void
XboxGenericMsg::set_axis_max(XboxAxis axis, int value)
{
}
  
/* EOF */

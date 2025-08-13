/*
	NoZ Game Engine
	
	Copyright(c) 2025 NoZ Games, LLC
*/

#pragma once

namespace noz
{
	// Unified input code for all input types
	enum class InputCode
	{
		None = 0,
		
		// Mouse Buttons
		MouseLeft,
		MouseRight,
		MouseMiddle,
		MouseButton4,
		MouseButton5,
		
		// Mouse Axes
		MouseX,
		MouseY,
		MouseScrollX,
		MouseScrollY,
		
		// Keyboard Keys (most common ones, can be expanded)
		KeyA, KeyB, KeyC, KeyD, KeyE, KeyF, KeyG, KeyH, KeyI, KeyJ, KeyK, KeyL, KeyM,
		KeyN, KeyO, KeyP, KeyQ, KeyR, KeyS, KeyT, KeyU, KeyV, KeyW, KeyX, KeyY, KeyZ,
		
		Key0, Key1, Key2, Key3, Key4, Key5, Key6, Key7, Key8, Key9,
		
		KeySpace,
		KeyEnter,
		KeyTab,
		KeyBackspace,
		KeyEscape,
		
		KeyLeftShift,
		KeyRightShift,
		KeyLeftCtrl,
		KeyRightCtrl,
		KeyLeftAlt,
		KeyRightAlt,
		
		KeyUp,
		KeyDown,
		KeyLeft,
		KeyRight,
		
		KeyF1, KeyF2, KeyF3, KeyF4, KeyF5, KeyF6,
		KeyF7, KeyF8, KeyF9, KeyF10, KeyF11, KeyF12,
		
		// Gamepad Buttons (works for any connected gamepad)
		GamepadA,           // Cross on PS, A on Xbox
		GamepadB,           // Circle on PS, B on Xbox
		GamepadX,           // Square on PS, X on Xbox
		GamepadY,           // Triangle on PS, Y on Xbox
		
		GamepadLeftShoulder,
		GamepadRightShoulder,
		GamepadLeftTriggerButton,  // L2/LT pressed as button
		GamepadRightTriggerButton, // R2/RT pressed as button
		
		GamepadStart,
		GamepadBack,        // Select/View
		GamepadGuide,       // PS/Xbox button
		
		GamepadLeftStickButton,
		GamepadRightStickButton,
		
		GamepadDPadUp,
		GamepadDPadDown,
		GamepadDPadLeft,
		GamepadDPadRight,
		
		// Gamepad Axes
		GamepadLeftStickX,
		GamepadLeftStickY,
		GamepadRightStickX,
		GamepadRightStickY,
		GamepadLeftTrigger,   // 0 to 1
		GamepadRightTrigger,  // 0 to 1
		
		// Specific gamepad versions (for when you need a specific controller)
		Gamepad1_A, Gamepad1_B, Gamepad1_X, Gamepad1_Y,
		Gamepad1_LeftShoulder, Gamepad1_RightShoulder,
		Gamepad1_Start, Gamepad1_Back,
		Gamepad1_LeftStickButton, Gamepad1_RightStickButton,
		Gamepad1_DPadUp, Gamepad1_DPadDown, Gamepad1_DPadLeft, Gamepad1_DPadRight,
		Gamepad1_LeftStickX, Gamepad1_LeftStickY,
		Gamepad1_RightStickX, Gamepad1_RightStickY,
		Gamepad1_LeftTrigger, Gamepad1_RightTrigger,
		
		Gamepad2_A, Gamepad2_B, Gamepad2_X, Gamepad2_Y,
		Gamepad2_LeftShoulder, Gamepad2_RightShoulder,
		Gamepad2_Start, Gamepad2_Back,
		Gamepad2_LeftStickButton, Gamepad2_RightStickButton,
		Gamepad2_DPadUp, Gamepad2_DPadDown, Gamepad2_DPadLeft, Gamepad2_DPadRight,
		Gamepad2_LeftStickX, Gamepad2_LeftStickY,
		Gamepad2_RightStickX, Gamepad2_RightStickY,
		Gamepad2_LeftTrigger, Gamepad2_RightTrigger,
		
		Gamepad3_A, Gamepad3_B, Gamepad3_X, Gamepad3_Y,
		Gamepad3_LeftShoulder, Gamepad3_RightShoulder,
		Gamepad3_Start, Gamepad3_Back,
		Gamepad3_LeftStickButton, Gamepad3_RightStickButton,
		Gamepad3_DPadUp, Gamepad3_DPadDown, Gamepad3_DPadLeft, Gamepad3_DPadRight,
		Gamepad3_LeftStickX, Gamepad3_LeftStickY,
		Gamepad3_RightStickX, Gamepad3_RightStickY,
		Gamepad3_LeftTrigger, Gamepad3_RightTrigger,
		
		Gamepad4_A, Gamepad4_B, Gamepad4_X, Gamepad4_Y,
		Gamepad4_LeftShoulder, Gamepad4_RightShoulder,
		Gamepad4_Start, Gamepad4_Back,
		Gamepad4_LeftStickButton, Gamepad4_RightStickButton,
		Gamepad4_DPadUp, Gamepad4_DPadDown, Gamepad4_DPadLeft, Gamepad4_DPadRight,
		Gamepad4_LeftStickX, Gamepad4_LeftStickY,
		Gamepad4_RightStickX, Gamepad4_RightStickY,
		Gamepad4_LeftTrigger, Gamepad4_RightTrigger,
		
		Count
	};
	
	// Helper functions
	inline bool IsKeyboardCode(InputCode code)
	{
		return code >= InputCode::KeyA && code <= InputCode::KeyF12;
	}
	
	inline bool IsMouseCode(InputCode code)
	{
		return code >= InputCode::MouseLeft && code <= InputCode::MouseScrollY;
	}
	
	inline bool IsGamepadCode(InputCode code)
	{
		return code >= InputCode::GamepadA && code <= InputCode::Gamepad4_RightTrigger;
	}
	
	inline bool IsAxisCode(InputCode code)
	{
		return code == InputCode::MouseX || code == InputCode::MouseY ||
		       code == InputCode::MouseScrollX || code == InputCode::MouseScrollY ||
		       (code >= InputCode::GamepadLeftStickX && code <= InputCode::GamepadRightTrigger) ||
		       (code >= InputCode::Gamepad1_LeftStickX && code <= InputCode::Gamepad1_RightTrigger) ||
		       (code >= InputCode::Gamepad2_LeftStickX && code <= InputCode::Gamepad2_RightTrigger) ||
		       (code >= InputCode::Gamepad3_LeftStickX && code <= InputCode::Gamepad3_RightTrigger) ||
		       (code >= InputCode::Gamepad4_LeftStickX && code <= InputCode::Gamepad4_RightTrigger);
	}
	
	inline bool IsButtonCode(InputCode code)
	{
		return !IsAxisCode(code) && code != InputCode::None;
	}
	
} // namespace noz
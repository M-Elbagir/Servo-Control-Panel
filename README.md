# Servo Control Panel Project

This project lets a web page control a servo motor through Supabase and an ESP32.

The flow is simple:

Browser -> Supabase -> ESP32 -> Servo

## How It Works

1. The browser sends the selected angle to the `servo_state` table in Supabase.
2. The ESP32 reads the latest angle from Supabase.
3. The ESP32 moves the servo to that angle.

## Supabase Requirements

The project expects a table named `servo_state` with these columns:

- `id`
- `angle`
- `updated_at`

The active row used by the project is `id = 'servo'`.

## Configuration

Update these values in the web page and ESP32 code:

- Supabase URL
- Supabase anon key
- Wi-Fi name
- Wi-Fi password

## Usage

1. Open the hosted control panel at https://mohamed-elbagir.free.je/servo%20control%20panel.
2. Move the slider or use voice input.
3. The servo should follow the latest angle stored in Supabase.

## Notes

- InfinityFree is used to host the web control panel.
- The browser writes the angle directly to Supabase.
- The ESP32 polls Supabase over HTTPS and moves the servo when the angle changes.
- Stable Wi-Fi improves responsiveness and reduces connection failures.

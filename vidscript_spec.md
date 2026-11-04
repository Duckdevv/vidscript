# Hou to use VidScript

## Introduction

VidScript is a code-driven motion graphics and video editing language. Instead of using a traditional visual timeline, everything is powered by precise scripts.

1. You create a project using the `Create()` function.
2. Every asset you use is assigned to a variable (e.g., `$my_asset = load("path/to/asset")`).
3. The `main{}` function acts as your timeline, where you define every keyframe and movement.
4. Every line inside the `main{}` function must target a variable starting with `$`.
5. Every line of code must end with `!`.

## setup & config

* to creat a new project the script shoudl start with :
    ```
     Create(name,duration,dimensions,fps)!
1. name is the name of the project .
2. duration is the final vidieo or the space you will be working with (min:sec) .
3. dimensions are the sise of the canvas you will be working with (x_pixels,y_pixels) .
4. fps are the vidieo frame per second .
    * like :
        ``` 
         Create("My Project",1:00:00,(1920,1080),30)!
Markdown
# VidScript Specification

## Introduction

VidScript is a code-driven motion graphics and video editing language. Instead of using a traditional visual timeline, everything is controlled through precise, simple scripts.

1. You create a project using the `Create()` function.
2. Every asset you use is assigned to a variable starting with `$` (e.g., `$my_asset = load("path/to/asset")`).
3. The `main{}` function acts as your timeline, where you write every action and keyframe.
4. Every line inside the `main{}` function must target an asset variable starting with `$`.
5. Every statement must end with `!`. Anything after `!` on the same line is treated as a comment.

---

## Basic Syntax Rules

* **Statement Terminator:** Every command or variable assignment must end with an exclamation mark `!`.
* **Comments:** Any text following `!` on the same line is ignored by the parser.
* **Asset Variables:** Variables referencing visual, audio, or text assets must start with the `$` sign (e.g., `$logo`, `$bg_video`).
* **Global Scope:** Asset definitions, shaders, and global setup occur outside the `main{}` function.
* **Timeline Scope:** All timeline movements, property changes, and keyframe definitions must be written inside the `main{}` block.

---

## Setup & Config (`Create` Function)

To start a new project, every VidScript file must begin with the `Create()` function at the top of the script. This sets up the rendering canvas and timeline limits.

### Syntax:
```
Create(name, duration, resolution)!
```
### Parameters:
name (String): The title of your project or output file (e.g., "My Project").

duration (Timestamp): The total duration of the video formatted as mm:ss (e.g., 01:00:00 for 1 minute).

resolution (Resolution): The canvas dimensions written as WIDTHxHEIGHT in pixels (e.g., 1920x1080 or 1080x1920).

### Example:
```
Create("My Project", 01:00:00, 1920x1080)!
```

---

## Variable Declaration & Referencing

In VidScript, handling variables follows a strict and simple convention to separate definition from usage.

### Rules:
1. **Declaration (Creating a Variable):** Write the variable name directly without any symbols (e.g., `my_var = ...`).
2. **Referencing (Using an Existing Variable):** Whenever you want to recall, target, or pass an existing variable (inside `main{}` or within functions), you **must** prefix its name with `$` (e.g., `$my_var`).

---

### Syntax Examples:

#### 1. Defining a Variable (Global Scope)
```
my_image = load("assets/photo.png")!
my_video = load("assets/clip.mp4")!
```
2. Referencing a Variable (Inside Timeline or Functions)
```
main() {
    $my_image {
        (00:00, opacity, 1)
    }!
}
```
---

## Video Slicing & Cutting (`take` Function)

To cut a specific part from a video or split a large video into smaller clips, use the `take()` function. 

This creates a virtual reference clip from the original source without duplicating the video in memory.

### Syntax:
```
new_clip_name = take($source_video, start_time, end_time)!
```
### Parameters & Time Shortcuts:

`$source_video`: The original video variable reference (must start with $).

start_time: The beginning time of the segment. Can be a timestamp (mm:ss) or the keyword start.

end_time: The ending time of the segment. Can be:

A timestamp (e.g., 01:30).

The keyword end (to the very end of the video).

A relative duration using + followed by pure numbers representing seconds (e.g., +15 for 15 seconds, or +00:15 as timestamp duration).

### Examples:
1. Standard Cut ( Cut from minute 01:00 to 01:30)
```
raw_video = load("assets/interview.mp4")!
part_1 = take($raw_video, 01:00, 01:30)!
```
2. Using Keywords (start & end)
```
raw_video = load("assets/interview.mp4")!
intro = take($raw_video, start, 00:45)!
outro = take($raw_video, 02:00, end)!
```
3. Using Relative Duration (+ Pure Numbers)
Start at 01:00 and cut for a duration of 15 seconds (+15)
```
raw_video = load("assets/interview.mp4")!
short_clip = take($raw_video, 01:00, +15)!
```

---

## Text Generation (`@text` Generator)

The `@text` generator creates a text object as a visual node and assigns it to a variable. This node can later be animated and transformed inside the `main{}` function.

### Syntax:
```
text_variable = @text(size, color, font_path, stroke_width, stroke_color, content)!
```
### Parameters:
size (Number): The font size in pixels (e.g., 48).

color (Hex Color): The text fill color in hex format (e.g., #FFFFFF).

font_path (String): The relative path to the font file (e.g., "fonts/arial.ttf").

stroke_width (Number): The outline thickness in pixels (use 0 for no outline).

stroke_color (Hex Color): The outline color in hex format (e.g., #000000).

content (String): The actual text message enclosed in double quotes.

### Examples:
1. Basic Text Definition
```
title_text = @text(64, #FFFFFF, "fonts/inter.ttf", 0, #000000, "Hello World")!
```
2. Text with Outline
```
heading = @text(48, #FFCC00, "fonts/bold.ttf", 2, #000000, "VidScript Engine")!
```

---

## Shape Generation (`@shape` Generator)

The `@shape` generator creates vector geometric shapes (such as rectangles, circles, etc.) as visual nodes and assigns them to a variable.

### Syntax:
```
shape_variable = @shape("type", size, fill_color, stroke_width, stroke_color, border_radius)!
```
### Parameters:
"type" (String): The shape geometry type (e.g., "rect", "circle").

size (Dimensions or Radius):

For "rect": Width and height specified as WIDTHxHEIGHT (e.g., 800x400).

For "circle": A single number for the radius (e.g., 150).

fill_color (Hex Color): The inside color of the shape in hex format (e.g., #181825).

stroke_width (Number): The outline thickness in pixels (use 0 for no stroke).

stroke_color (Hex Color): The outline color in hex format (e.g., #FFFFFF).

border_radius (Number): Corner rounding in pixels (applicable to "rect", use 0 for sharp corners).

### Examples:
1. Rectangle Shape
```
card_bg = @shape("rect", 800x400, #181825, 2, #FFFFFF, 16)!
```
2. Circle Shape
```
red_dot = @shape("circle", 100, #FF0000, 0, #000000, 0)!
```

---

## Mask Generation (`@mask` Generator)

The `@mask` generator defines a clipping or masking shape used to cut out parts of other visual elements.

### Syntax:
```
mask_variable = @mask("type", size)!
```
### Parameters:
"type" (String): The geometry type of the mask (e.g., "circle", "rect").

size (Dimensions or Radius):

For "circle": A single number representing the radius (e.g., 150).

For "rect": Width and height specified as WIDTHxHEIGHT (e.g., 400x400).

### Examples:
1. Circle Mask
```
avatar_mask = @mask("circle", 150)!
```
2. Rectangle Mask
```
box_mask = @mask("rect", 400x400)!
```

---

## Camera Generation (`@camera` Generator)

The `@camera` generator defines a viewport frame that controls the visible area and perspective of the scene.

### Syntax:
```
camera_variable = @camera(resolution)!
```
### Parameters:
resolution (Dimensions): The viewport dimensions specified as WIDTHxHEIGHT in pixels (e.g., 1920x1080).

### Examples:
1. Standard Full HD Camera
```
main_cam = @camera(1920x1080)!
```
2. Vertical Camera (for Short Content)
```
reel_cam = @camera(1080x1920)!
```

---

## Applying Masks (`.use()` Function)

To crop or mask a visual element (image, video, shape, or text) using a previously defined `@mask`, use the `.use()` method on the target variable reference.

Since both the target element and the mask are existing variables, both **must** use the `$` prefix when referenced.

### Syntax:
```
$target_variable.use($mask_variable)!
```
### Parameters:
`$target_variable`: The reference to the target visual element (must start with $).

`$mask_variable`: The reference to the created @mask variable (must start with $).

### Examples:
1. Masking an Image with a Circle Mask
* Create a circle mask
```
avatar_mask = @mask("circle", 150)!
```
* Load the profile image
```
profile_pic = load("assets/user.png")!
```
* Apply the mask using $ for both existing variables
```
$profile_pic.use($avatar_mask)!
```
2. Masking a Video Clip
* Create a rectangular mask
```
box_mask = @mask("rect", 800x400)!
```
* Load and slice a video
```
my_video = load("assets/background.mp4")!
video_clip = take($my_video, 00:00, 00:10)!
```
* Apply the mask using $ for both existing variables
```
$video_clip.use($box_mask)!
```

---

## The Main Timeline (`main()` Function)

The `main()` function is the core execution block and timeline container of VidScript. Everything that happens over time (animations, entries, exits, property modifications) must be placed inside this function.

### Key Rules:
1. **Global Timeline Scope:** All animation statements and scene instructions must be written within the curly braces `{}` of `main()`.
2. **Targeting Assets:** To apply changes to any previously defined asset, you target its variable name starting with `$`.
3. **Asset Scope Blocks:** Inside `main()`, each targeted asset has its own block `{}` where its timeline events are written.

---

### Basic Structure Syntax:

```
main() {
    $first_asset {
    }!

    $second_asset {
    }!
}
```
### Example (Structure Only):

1. Assets defined in global scope
```
bg_image = load("assets/bg.png")!
title_text = @text(48, #FFFFFF, "fonts/inter.ttf", 0, #000000, "VidScript")!
```
2. Timeline structure
```
main() {
    $bg_image {
    }!

    $title_text {
    }!
}
```
---

## Asset Actions & Execution Flow

Inside the asset block `{}` in `main()`, every action or animation command is enclosed in parentheses `()`.

### Rules:
1. **Action Format:** Every single action must be wrapped in standard parentheses `( ... )`.
2. **Action Separator (`;`):** Actions inside the same asset block are separated by a semicolon `;`.
3. **Sequential Execution:** Actions execute in order from left to right / top to bottom based on the semicolon `;` separator.

---

### Action Syntax Structure:

```
$asset_name {
    (Action 1);
    (Action 2);
    (Action 3)
}!
```
### Example (Flow Concept):
```
main() {
    $title_text {
        (First Action);
        (Second Action);
        (Third Action)
    }!
}
```

---

## Instant Action (Direct Property Change)

An instant action changes a property value immediately at a single specific timestamp. It takes no time to execute and has no transition duration.

### Syntax:
```
(timestamp, property_name, value)
```
### Parameters:
timestamp: The exact point in time when the change occurs, formatted as mm:ss (e.g., 00:02:00).

property_name: The target attribute to modify (e.g., opacity, position, scale).

value: The new value assigned to the property immediately at that timestamp.

Example:
```
main() {
    $logo {
        (00:00.00, opacity, 0);
        (00:05.00, opacity, 1)  
    }!
}
```
---

---

## Linear & Eased Transition Action (Gradual Change)

Transitions in vidscript handle both simple linear movements and complex motion-graphics easing curves (Easing In/Out) directly within the transition syntax using curve direction indicators (`/` and `\`) alongside strength values.

---

### Transition Types & Easing Syntax:

1. Linear Transition (Constant Speed)
Uses a straight vertical bar `|` between values for a uniform, flat speed transition.
```
(start_time/end_time, property_name, start_value | end_value)
```
2. Ease In (Accelerate)
Uses a forward slash / to represent an upward accelerating curve.
```
(start_time/end_time, property_name, start_value / strength / end_value)
```
3. Ease Out (Decelerate)
Uses a backslash \ to represent a slowing down curve.
```
(start_time/end_time, property_name, start_value \ strength \ end_value)
```
4. Ease In-Out (Accelerate then Decelerate)
Combines forward slash / and backslash \ to form a classic S-curve shape.
```
(start_time/end_time, property_name, start_value / strength \ end_value)
```
5. Ease Out-In (Decelerate then Accelerate)
Combines backslash \ and forward slash / for inverted curve motion.
```
(start_time/end_time, property_name, start_value \ strength / end_value)
```
### Parameter Breakdown:
strength (Float/Integer): Defines the intensity/curvature of the easing curve (similar to graph speed handles). Higher numbers create steeper acceleration/deceleration.

pass Keyword: Can still be used as start_value to automatically inherit the current property value before applying the easing curve.

Examples:
```
main() {
    $logo {
        (00:00.00/00:02.00, opacity, 0 | 1);

        (00:02.00/00:04.00, scale, 1.0 / 5 \ 2.0);

        (00:05.00/00:06.00, opacity, pass \ 3 \ 0)
    }!
}
```

## Asset Properties & Parameters

Every visual element in VidScript possesses a set of controllable properties that can be animated inside `main()`. 

* **Common Parameters:** Core properties shared across all visual assets (such as position, opacity, scale, and rotation).
* **Specific Parameters:** Unique properties dedicated exclusively to certain asset types (such as font parameters for text, or dimensions for shapes).

---

## Image Asset Parameters

Images support a core set of transformation, layer, and visibility properties inside `main()`.

---

### Parameters List:

#### 1. `opacity` *(Float)*
Controls the transparency level of the image.
* **Value Range:** `0.0` (completely invisible) to `1.0` (fully visible).

#### 2. `layer` *(Integer)*
Defines the Z-index rendering order when multiple assets overlap.
* **Value:** An integer number starting from `0` (bottom-most layer). Higher numbers appear on top of lower numbers.

#### 3. `size` *(Float)*
Scales the image uniformly while strictly maintaining its original aspect ratio.
* **Value:** `1.0` represents original 100% dimensions. Values greater than `1.0` enlarge the image, while values less than `1.0` shrink it.

#### 4. `scale` *(Vector2: Float x Float)*
Scales the image non-uniformly along the X and Y axes (useful for stretching or squashing).
* **Format:** `X_val x Y_val` (e.g., `1.5 x 0.8`).

#### 5. `rotation` *(float)*
Rotates the image clockwise in degrees around its center.
* **format:** `00.0` (e.g., `90.0`, `180.5`, `-350.7`).

#### 6. `position` *(Vector2: Integer x Integer or Keywords)*
Determines the absolute coordinates of the image in pixels (`X x Y`).
* **Format:** `X_pixel x Y_pixel` (e.g., `960 x 540`).
* **Keywords:** Supports the `center` keyword on either axis to align automatically to the middle of the frame (e.g., `center x 200` centers the image horizontally at Y = 200).

---

### Examples:

```
main() {
    $my_image {
        (00:00.00, layer, 2);
        (00:00.00, opacity, 1.0);
        (00:00.00, position, center x 300);
        (00:01:00, rotation, 45);
        (00:01:00, scale, 2.0 x 1.0)
    }!
}
```

---

## Video Asset Parameters

Video assets inherit all fundamental visual properties from images (`opacity`, `layer`, `size`, `scale`, `rotation`, `position`) and add dedicated controls for playback and audio handling inside `main()`.

---

### Inherited Visual Parameters:
* **`opacity`** *(Float)*: `0.0` to `1.0`.
* **`layer`** *(Integer)*: Z-index layer order (`0` is bottom).
* **`size`** *(Float)*: Uniform scaling (`1.0` is original size).
* **`scale`** *(Vector2: Float x Float)*: Non-uniform scaling (`X x Y`).
* **`rotation`** *(float)*: Rotation angle in degrees .
* **`position`** *(Vector2: Integer x Integer or Keywords)*: Coordinates in pixels (`X x Y` or using `center`).

---

### Video-Specific Parameters:

#### 1. `speed` *(Float)*
Controls the playback speed rate of the video.
* **Default:** `1.0` (normal speed).
* **Values:** Higher values accelerate playback (e.g., `2.0` for 2x speed), while lower values create slow motion (e.g., `0.5`).

#### 2. `pause` *(Boolean)*
Freezes playback on the current frame.
* **Default:** `false` (video plays automatically).
* **Values:** `true` pauses playback and freezes the current frame, while `false` resumes normal video playback.

#### 3. `mute` *(Boolean)*
Mutes or unmutes the video's embedded audio track.
* **Default:** `false` (audio enabled).
* **Values:** `true` silences the video audio, while `false` enables audio playback.

#### 4. `volume` *(Float)*
Adjusts the audio volume level of the video track.
* **Default:** `1.0` (original volume level).
* **Values:** Higher values boost volume, while lower values decrease it (`0.0` is silent).

#### 5. `pitch_scale` *(Float)*
Modifies the audio pitch scale without affecting playback speed independently.
* **Default:** `1.0` (original pitch).
* **Values:** Decimal multipliers adjusting the audio pitch frequency higher or lower.

---

### Example:

```
main() {
    $intro_video {
        (00:00.00, speed, 2.0);
        (00:00.00, volume, 0.5);
        (00:05.00, pause, true);
        (00:05.00, mute, true)
    }!
}
```
---

## Audio Asset Parameters

Audio assets handle background music, voiceovers, and sound effects inside `main()`. They control volume, pitch, playback states, speed, and positioning.

---

### Parameters List:

#### 1. `volume` *(Float)*
Adjusts the audio loudness level.
* **Default:** `1.0` (original volume level).
* **Values:** Higher values boost volume, while lower values decrease it (`0.0` is silent).

#### 2. `speed` *(Float)*
Controls the playback speed of the audio track without affecting its pitch (independent time-stretching).
* **Default:** `1.0` (normal speed).
* **Values:** Higher values accelerate audio, lower values slow it down.

#### 3. `pitch_scale` *(Float)*
Modifies the pitch frequency of the audio track independently without changing its speed.
* **Default:** `1.0` (original pitch).
* **Values:** Decimal multipliers adjusting the audio pitch higher or lower.

#### 4. `pause` *(Boolean)*
Freezes or resumes audio playback.
* **Default:** `false` (audio plays automatically).
* **Values:** `true` pauses audio, while `false` resumes playback.

#### 5. `pointer` *(Timestamp)*
Jumps/seeks the audio playhead directly to a specific timestamp within the audio file (`mm:ss`).
* **Format:** Timestamp value (e.g., `00:15.00` seeks to the 15th second of the track).

#### 6. `replay` *(Boolean)*
Triggers the audio track to instantly restart playback from its beginning (or current `pointer`) at the specified timeline timestamp. Ideal for repeating sound effects without cloning variables.
* **Default:** `false`.
* **Values:** Setting to `true` instantly re-triggers the sound. Setting to `false` does nothing.

---

### Example:

```
bg_music = load("audio/music.mp3")!

main() {
    $bg_music {
        (00:00.00, pointer, 00:30.00);
        (00:00.00, volume, 1.0);
        (00:10.00, speed, 1.5);
        (00:20.00, replay, true)
    }!
}
```
---

## Mask Asset Parameters

Mask objects defined with `@mask` support a set of visual and logic control parameters inside `main()` to dynamically manipulate clipping regions over time.

---

### Parameters List:

#### 1. `size` *(Float)*
Scales the mask geometry uniformly while maintaining its aspect ratio.
* **Default:** `1.0` (original defined mask dimensions).
* **Values:** Values greater than `1.0` expand the mask, while values less than `1.0` shrink it.

#### 2. `scale` *(Vector2: Float x Float)*
Scales the mask non-uniformly along X and Y axes to stretch or squash the clipping shape.
* **Format:** `X_val x Y_val` (e.g., `1.5 x 0.5`).

#### 3. `position` *(Vector2: Integer x Integer or Keywords)*
Controls the relative offset coordinates of the mask over the target element.
* **Format:** `X_pixel x Y_pixel` (e.g., `100 x 200`).
* **Keywords:** Supports `center` to position the mask dead-center on the specified axis (e.g., `center x center`).

#### 4. `opacity` *(Float)*
Controls the strength and visibility threshold of the masking effect.
* **Value Range:** `0.0` (disables the mask effect entirely) to `1.0` (full strength clipping).

#### 5. `invert` *(Boolean)*
Inverts the masking logic (creates a cutout or negative mask).
* **Default:** `false` (standard mask: reveals content inside the mask shape, hides outside).
* **Values:** `true` hides content inside the mask shape and reveals everything outside. `false` restores default masking behavior.

---

### Example:

```
avatar_mask = @mask("circle", 150)!
profile_pic = load("assets/user.png")!

$profile_pic.use($avatar_mask)!

main() {
    $avatar_mask {
        (00:00.00, position, center x center);
        (00:00.00, opacity, 1.0);
        (00:03.00, invert, true);
        (00:03.00/00:05.00, size, 1.0/2.5)
    }!
}
```
---

## Text Asset Parameters

Text objects created with `@text` inherit standard visual transformations (`opacity`, `size`, `scale`, `position`, `rotation`) and add controls for text content, styling, and strokes inside `main()`.

---

### Parameters List:

#### 1. `opacity` *(Float)*
Controls the transparency level of the text.
* **Value Range:** `0.0` (invisible) to `1.0` (fully visible).

#### 2. `size` *(Float)*
Scales the text font size uniformly.
* **Value:** `1.0` is the base defined font size. Multipliers scale the text up or down.

#### 3. `scale` *(Vector2: Float x Float)*
Scales the text non-uniformly along X and Y axes (useful for stretching text).
* **Format:** `X_val x Y_val` (e.g., `1.2 x 1.0`).

#### 4. `position` *(Vector2: Integer x Integer or Keywords)*
Sets the coordinates of the text box (`X x Y` or using `center`).

#### 5. `rotation` *(float)*
Rotates the text in degrees.

#### 6. `color` *(Hex Color)*
Changes the fill color of the text.
* **Format:** Hex color code (e.g., `#FF5500`).

#### 7. `font` *(String)*
Changes the font family file path dynamically.
* **Format:** Relative file path in quotes (e.g., `"fonts/bold.ttf"`).

#### 8. `stroke_width` *(Number)*
Controls the outline thickness around the text in pixels (use `0` to remove outline).

#### 9. `stroke_color` *(Hex Color)*
Changes the color of the text outline.
* **Format:** Hex color code (e.g., `#000000`).

#### 10. `content` *(String - Instant Action Only)*
Updates the actual displayed text message dynamically.
* **Note:** Because string values cannot be linearly interpolated, this property **only supports Instant Actions** (direct changes at a specific timestamp).

---

### Example:

```
title_text = @text(48, #FFFFFF, "fonts/inter.ttf", 0, #000000, "Hello")!

main() {
    $title_text {
        (00:00.00, position, center x center);
        (00:00.00, color, #FFFFFF);
        (00:02.00, content, "hello world");
        (00:02.00/00:04.00, color, #FFFFFF/#FFD700)
    }!
}
```
---

## Shape Asset Parameters

Shape objects created with `@shape` inherit all fundamental visual transform properties from images (`opacity`, `layer`, `size`, `scale`, `rotation`, `position`) and provide specific controls for fill color, border radius, and stroke styling inside `main()`.

---

### Inherited Visual Parameters:
* **`opacity`** *(Float)*: `0.0` to `1.0`.
* **`layer`** *(Integer)*: Z-index rendering order (`0` is bottom-most).
* **`size`** *(Float)*: Uniform scaling (`1.0` is base size).
* **`scale`** *(Vector2: Float x Float)*: Non-uniform scaling (`X x Y`).
* **`rotation`** *(float)*: Rotation angle in degrees .
* **`position`** *(Vector2: Integer x Integer or Keywords)*: Coordinates in pixels (`X x Y` or using `center`).

---

### Shape-Specific Parameters:

#### 1. `color` *(Hex Color)*
Changes the solid fill color of the shape.
* **Format:** Hex color code (e.g., `#FF3366`).

#### 2. `border_radius` *(Number)*
Controls the corner rounding radius in pixels (applicable to rectangular shapes).
* **Value:** Number of pixels (use `0` for sharp 90-degree corners).

#### 3. `stroke_width` *(Number)*
Controls the outer outline border thickness in pixels.
* **Value:** Thickness in pixels (use `0` for no stroke).

#### 4. `stroke_color` *(Hex Color)*
Changes the color of the shape's outline border.
* **Format:** Hex color code (e.g., `#FFFFFF`).

---

### Example:

```
box = @shape("rect", 400x200, #181825, 2, #FFFFFF, 12)!

main() {
    $box {
        (00:00.00, position, center x center);
        (00:00.00, layer, 1);
        (00:01.00/00:03.00, color, #181825/#FF5500);
        (00:01.00/00:03.00, border_radius, 12/50)
    }!
}
```
---

## Camera Parameters (`@camera`)

The camera controls the viewport frame of the entire scene inside `main()`. Changing camera properties affects all rendered visual elements simultaneously.

> **Camera Rules:**
> 1. **Single Instance Limit:** You can define and use **only one camera** per project.
> 2. **Optional Asset:** Adding a camera is optional; if omitted, the project renders from a fixed default full-screen viewport.
> 3. **No Layering:** The camera does **not** support the `layer` parameter as it always operates above all layers to capture the unified frame.

---

### Parameters List:

#### 1. `position` *(Vector2: Integer x Integer or Keywords)*
Pan and move the viewpoint across the entire scene (`X x Y`).
* **Format:** `X_pixel x Y_pixel` or using the `center` keyword.

#### 2. `zoom` *(Float)*
Zooms the entire scene in or out smoothly.
* **Default:** `1.0` (standard 100% viewport zoom).
* **Values:** Multipliers greater than `1.0` zoom into the scene, while values less than `1.0` zoom out.

#### 3. `opacity` *(Float)*
Controls global visibility for the entire output viewport (acts as a master scene blackout/fade switch).
* **Value Range:** `0.0` (blackout / camera off) to `1.0` (full scene visibility).

#### 4. `rotation` *(float)*
Rotates the entire framed scene clockwise in degrees .

---

### Example:

```
main_cam = @camera(1920x1080)!

main() {
    $main_cam {
        (00:00.00, position, center x center);
        (00:00.00, opacity, 1.0);
        (00:01.00/00:04.00, zoom, 1.0/1.5);
        (00:05.00, rotation, 90)
    }!
}
```
---

## Reusable Action Functions (`func`)

To keep code DRY (Don't Repeat Yourself) and avoid duplicating complex animation sequences across multiple assets, vidscript allows defining external action functions outside of `main()`.

Action functions encapsulate a chain of timeline actions that can be invoked directly as a single action line inside any asset block.

### Rules:
1. **Global Declaration:** Functions are defined outside the `main()` block (typically placed at the bottom of the script).
2. **Exclamation Termination:** The function declaration block must end with an exclamation mark (`}!`).
3. **Internal Semicolons:** Semicolons `;` must separate actions inside the function body.
4. **Direct Invocation:** Inside an asset block in `main()`, call the function using standard parenthesized call syntax as an action step (e.g., `func_name()`).

---

### Syntax:

#### Function Definition (Outside `main()`):
```
func function_name() {
    (action_1);
    (action_2);
    (action_3)
}!
```
### Function Call (Inside main()):
```
main() {
    $target_asset {
        (pre_action);
        function_name();
        (post_action)
    }!
}
```
### Example:
```
logo = load("assets/logo.png")!
card = load("assets/card.png")!

main() {
    $logo {
        (00:00.00, position, center x center);
        pop_and_fade();
        (00:05.00, layer, 3)
    }!

    $card {
        (00:02.00, position, 100 x 200);
        pop_and_fade()
    }!
}

func pop_and_fade() {
    (00:00.00/00:01.00, opacity, 0.0/1.0);
    (00:01.00/00:02.00, size, 1.0/1.3);
    (00:02.00/00:03.00, opacity, 1.0/0.0)
}!
```
---

## Custom Variables & Constants

To write cleaner code and avoid repeating hardcoded values (such as specific positions, colors, speed ratios, or text strings), VidScript allows defining custom variables in the global scope.

### Core Rules:
1. **Immutable by Design (Constants):** All variables in VidScript are strictly **immutable constants**. Once defined, their value cannot be reassigned or modified later in the script.
2. **Global Declaration:** Custom variables are declared outside of `main()` without using any `var` or `let` keywords simply state the variable name followed by `=`.
3. **Reference with `$`:** Just like built-in assets, any custom variable must be referenced using the `$` prefix when used inside actions or generators.

---

### Supported Data Types:

* **Strings:** `"Hello World"`
* **Numbers / Floats:** `1.5`, `300`, `0.5`
* **Vectors:** `1920x1080`, `2.0x1.5`
* **Colors:** `#FF5500`
* **Timestamps:** `00:02.00`

---

### Syntax & Example:

```
main_color = #FF5500!
center_y = 540!
default_text = "Hello World"!
hero_speed = 1.5!

title_text = @text(64, $main_color, "assets/font.ttf", 0, #000000, $default_text)!
hero_video = load("assets/video.mp4")!

main() {
    $title_text {
        (00:00.00, position, center x $center_y); (00:01.00/00:03.00, color,$main_color/#FFFFFF)
    }!

    $hero_video { (00:00.00, speed,$hero_speed)
    }!
}
```
---


## Core Syntax Rules & Constraints

To ensure smooth timeline execution and prevent parsing conflicts in VidScript, all scripts must follow these core constraints:

---

### 1. Per-Asset Timeline Continuity (Chronological Order)

Timeline progression rules apply **strictly per individual asset**, not globally across different assets.

* **Asset Self-Continuity:** Inside a single asset's block (`$asset { ... }`), action timestamps must strictly move forward or remain equal. An asset cannot trigger an action at `00:02.00` after it has already performed an action at `00:05.00`.
* **Cross-Asset Independence:** Different assets operate independently on the timeline. Multiple assets can trigger actions at the exact same timestamp simultaneously (e.g., `$image1` and `$image2` both starting a fade-in at `00:00.00`).

---

### 2. Isolated Asset Scope

Every asset must encapsulate its actions within its own dedicated block inside `main()`.

* **Syntax:** Target the asset using `$asset_name` followed by a `{}` block containing its actions.
* **Separation:** Each asset declaration starts on its own line and closes with `}!`.

---

### 3. Exclamation Termination (`!`)

Every block or global statement must terminate with an exclamation mark `!`.

* Asset blocks inside `main()` end with `}!`.
* Functions outside `main()` end with `}!`.
* Global variable and asset declarations end with `!`.

---

### Example (Multi-Asset Parallel Timeline):

```
bg_image = load("assets/bg.png")!
title_text = @text(48, #FFFFFF, "assets/font.ttf", 0, #000000, "Hello World")!

main() {
    $bg_image {
        (00:00.00/00:02.00, opacity, 0.0/1.0);
        (00:05.00, opacity, 0.0)
    }!

    $title_text {
        (00:00.00, opacity, 1.0);
        (00:02.00/00:04.00, scale, 1.0/1.5)
    }!
}
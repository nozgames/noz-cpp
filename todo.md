- [ ] assert if scratch is in thread, handle scratch in threads
- [ ] anti-alias render
- [ ] outline color in mesh editor



# incoming
- [ ] replace randomint with min-max exclusive 
- [ ] color override push in render buffer (PushColorOverride / PopColorOverride)
- [ ] notifcation ui is not aligning to bottom right
- [ ] build tooltip detection into ui system (IsHoverTooltip?)
- [ ] text mesh should not include font size, instead lets scale the mesh
- [ ] text mesh no hash just list search for hash
- [ ] text mesh should remove any that were not used last frame
- [ ] write palettes to asset manifest

# UI
- [ ] Begin/End Popup
  - [ ] Anchor Align
- [ ] hash the entity styles to see if they have changed, if not then dont redo transforms
- [ ] optimize container alignment when top-left is being used.

# Sound
- [ ] sound editor so you can adjust values such as random pitch, voluem ,etc
 
# Mesh
- [ ] opacity selector 
- [ ] do not allow extruding an internal edge (check face count)

# Atlas
- [ ] Hover on atlas rect should highlight the asset 

# Skeleton

- [ ] higlight asset on hover in select tool
- [ ] Creating a new skeleton dos not appear until restarting
- [ ] After altering bone transforms in the skeleton the animations do not update until going into edit mode on the animation
- [ ] Adding a bone to the skeleton caused the animations to write garbage data for position
- [ ] moving bones in skeleton is not changing animations?
- [ ] hit test faces of assets when parenting.

# State
- [ ] Add state that works like event except an animation can be in multiple states (hidden arrow for example)

# Event
- [ ] Deleting an event should go through all animations and remove the event
- [ ] Write event ids as constants in the manifest
- [ ] dont include events in builds
- [ ] new event should find a unique id based on current events
- [ ] editor and inspector to allow you to set the id manually?

# Shader
- [ ] Dont crash if shader fails to compile

# Animation

- [ ] remove root motion flag and metadata
- [ ] when root motion is disabled the bounds is incorrectly calculated
- [ ] bounds needs to include skinned mesh extents
- [ ] Skinned mesh setup for animation
  - [ ] attach animation to animation (bow as child of stick_bow_fire)
- [ ] Animation play without being in editor
- [ ] Animation bounds does not take into account the actual animation, just the skeleton
- [ ] new clone animation
- [ ] Animation with no frames not loading
- [ ] hover on animation should play ?

# VFX
- [ ] custom mesh
- [ ] vfx hotload broken ?
- [ ] vfx bounds is not correct
- [ ] Trail vfx for individual particles
- [ ] hover to play
- [ ] depth sort particles
- [ ] updating particle files does not seem to update in the game immediately (have to save twice)

# Sound
- [ ] hot load of sounds
- [ ] way to stop sound when playing (music for example)

# Animated Mesh
- [ ] hold frames not working
- [ ] play animated mesh as vfx (one time, stop after play)

# General

- [ ] Stream FPS to the editor and any other stats 
- [ ] name pan in tools like knife and select
- [ ] Crash when typing text
- [ ] palette edtior?
- [ ] hotkey to select all connected geometry (like in blender)
- [ ] ctrl-d to duplicate selected mesh face
- [ ] multi-select delete did not work (make sure we remove all the assets first then remove the files)
- [ ] Can create the name of an existing asset and it doubles it up in memory
- [ ] Type numbers for g when moving assets
- [ ] Capture mouse while using tools like grab
- [ ] Prevent duplicate assets of same type and same name in different folders.
- [ ] g r to reset / r r rather than alt-g and alt-r
- [ ] Saving an asset is causing a double import (.meta?)
- [ ] Saving an asset should suppress the import notification 
- [ ] Need way to handle invalid assets (show invalid or error in editor, animation with missing skeleton for example)
- [ ] No way to see log messages anymore, we need a console window (win32 window you can open?)
- [ ] Alt-drag on asset to create instance  (draw origin in blue and outline a lighter grey)
- [ ] Use TextInput for typing after move or scale command
- [ ] Pressing g or r should stop the previous g or r and start a new one
- [ ] x to limit move
- [ ] y to limit move
- [ ] vfx initial_rotation and rotation over time
- [ ] scale command which allows you to specify a scale value for assets
- [ ] Something causes a spawn of error messages in the editor when client disconnects / reconnects, not sure which
- [ ] combined vertex buffer for rendering so we can stream data to it like triangles?
- [ ] convert props to be struct
- [ ] Trace a font out with meshes?
- [ ] Auto complete on commands
- [ ] Change UI, vertex size, edge size, etc to not scale with the window size
- [ ] Localized strings

# Skins

- [ ] new asset type for skin
- [ ] skeleton has a built in skin (default skin)
- [ ] skin editor allow adding or removing meshes to the skin
- [ ] meshes are designed at root
- [ ] meshes can be weighted to a single skeleton
- [ ] can edit weights of meshes in animation editor or skeleton editor or skin editor
- [ ] each vertex can have up to 4 weights[ui.glsl](../../nonstick/noz/assets/shaders/ui.glsl)

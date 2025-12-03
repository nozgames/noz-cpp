# Mesh
- [ ] do not allow extridomg an internal edge (check face count)
- [ ] new mesh should use click to add verts to create shape, enter to close
- [ ] There are some bugs with deletting edges and vertices
- [ ] vertices left behind when deleting faces nad edges
- [ ] remove edge support
- [ ] key to hide / show palette (alt-c?)

# Skeleton

- [ ] event editor
- [ ] higlight asset on hover in select tool
- [ ] Creating a new skeleton dos not appear until restarting
- [ ] After altering bone transforms in the skeleton the animations do not update until going into edit mode on the animation
- [ ] Adding a bone to the skeleton caused the animations to write garbage data for position
- [ ] moving bones in skeleton is not changing animations?
- [ ] hit test faces of assets when parenting.

# Animation

- [ ] Loop toggle button
- [ ] bone selection with root motion disabled not working
- [ ] when root motion is disabled the bounds is incorrectly calculated
- [ ] Animation event editor (name events)
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

# Sound
- [ ] hot load of sounds
- [ ] way to stop sound when playing (music for example)

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

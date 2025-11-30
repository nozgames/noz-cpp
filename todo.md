# Skins

- [ ] new asset type for skin
- [ ] skeleton has a built in skin (default skin)
- [ ] skin editor allow adding or removing meshes to the skin
- [ ] meshes are designed at root
- [ ] meshes can be weighted to a single skeleton
- [ ] can edit weights of meshes in animation editor or skeleton editor or skin editor
- [ ] each vertex can have up to 4 weights[ui.glsl](../../nonstick/noz/assets/shaders/ui.glsl)

# Todo

- [ ] hotkey to hide/show grid (ctrl - ') 
- [ ] higlight asset on hover in select tool
- [ ] name pan in tools like knife and select
- [ ] Crash when typing text
- [ ] Skinned mesh setup for animation
  - [ ] attach animation to animation (bow as child of stick_bow_fire)

- [ ] palette edtior?
- [ ] hotkey to select all connected geometry (like in blender)
- [ ] ctrl-d to duplicate selected mesh face
- [ ] new mesh should use click to add verts to create shape, enter to close
- [ ] Animation play without being in editor
- [ ] multi-select delete did not work
- [ ] Dont draw origins of objects that are not being edited in front of those that are
- [ ] There are some bugs with creating faces
- [ ] There are some bugs with deletting edges and vertices
- [ ] do not allow estruding an internal edge

- [ ] Show border color in the color picker with a different box
- [ ] Creating a new skeleton dos not appear until restarting
- [ ] After altering bone transforms in the skeleton the animations do not update until going into edit mode on the animation
- [ ] Can create the name of an existing asset and it doubles it up in memory
- [ ] Type numbers for g when moving assets
- [ ] Animation event editor
- [ ] key to hide / show palette (alt-c?)
- [ ] Adding a bone to the skeleton caused the animations to write garbage data for position
- [ ] Capture mouse while using tools like grab
- [ ] Animation bounds does not take into account the actual animation, just the skeleton
- [ ] moving bones in skeleton is not changing animations?
- [ ] Prevent duplicate assets of same type and same name in different folders.
- [ ] editor animation likely leaking animation and skeleton asset 
- [ ] vfx hotload broken
- [ ] new clone animation
- [ ] g r to reset / r r rather than alt-g and alt-r
- [ ] Saving an asset is causing a double import (.meta?)
- [ ] Saving an asset should suppress the import notification 
- [ ] Need way to handle invalid assets (show invalid or error in editor, animation with missing skeleton for example)
- [ ] Animation with no frames not loading
- [ ] After creating new mesh I cannot select bones in the skeleton view
- [ ] First time you load the shader is not loaded and it messes up, we need to handle the hotload?
- [ ] No way to see log messages anymore, we need a console window (win32 window you can open?)
- [ ] when moving bones in animation we have to recalculate the local position from the world after delta change
- [ ] Selecting a mesh asset should account for border in selection
- [ ] Alt-drag on asset to create instance  (draw origin in blue and outline a lighter grey)
- [ ] Use TextInput for typing after move or scale command
- [ ] Pressing g or r should stop the previous g or r and start a new one
- [ ] Box select being pushed it making the color go away, is that ok?
- [ ] single click to select asset, ctrl to toggle
- [ ] x to limit move
- [ ] y to limit move
- [ ] vfx initial_rotation and rotation over time
- [ ] scale command which allows you to specify a scale value for assets

- [ ] Icons for assets that have no draw function
- [ ] Something causes a spawn of error messages in the editor when client disconnects / reconnects, not sure which
- [ ] combined vertex buffer for rendering so we can stream data to it like triangles?
- [ ] hot load of textures
- [ ] hot load of sounds
- [ ] convert props to be struct
- [ ] Trace a font out with meshes?
- [ ] Auto complete on commands
- [ ] Load editor assets in job
- [ ] Change UI, vertex size, edge size, etc to not scale with the window size
- [ ] Instance a skelton so we can parent different skinned meshes (sotre the instances in the meta file for the skeleton)
- [ ] hover on animation should play ?
- [ ] Trail vfx for individual particles
- [ ] Localized strings

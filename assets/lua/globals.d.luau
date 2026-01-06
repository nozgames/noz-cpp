-- Type definitions for NoZ Lua API

-- Asset types (opaque userdata)
declare class Font end
declare class Mesh end
declare class Texture end
declare class Sound end
declare class Skeleton end
declare class Animation end
declare class Shader end
declare class Bin end
declare class Vfx end

-- Asset loaders
declare function LoadMesh(path: string): Mesh
declare function LoadTexture(path: string): Texture
declare function LoadShader(path: string): Shader
declare function LoadFont(path: string): Font
declare function LoadSound(path: string): Sound
declare function LoadSkeleton(path: string): Skeleton
declare function LoadAnimation(path: string): Animation
declare function LoadVfx(path: string): Vfx

declare class Color
    r: number
    g: number
    b: number
    a: number
end

declare Color: {
    new: (r: number?, g: number?, b: number?, a: number?) -> Color,
}

export type Align = number

export type CanvasStyle = {
    type: number?,
    color: Color?,
    id: number?,
}

export type ContainerStyle = {
    width: number?,
    height: number?,
    color: Color?,
    align: Align?,
    id: number?,
    border_radius: number?,
    border_color: Color?,
    border_width: number?,
}

export type LabelStyle = {
    font: Font?,
    fontSize: number?,
    color: Color?,
    align: Align?,
}

export type ImageStretch = number

export type ImageStyle = {
    stretch: ImageStretch?,
    align: Align?,
    scale: number?,
    color: Color?,
}

declare function BeginCanvas(style: CanvasStyle?): ()
declare function EndCanvas(): ()
declare function BeginContainer(style: ContainerStyle?): ()
declare function EndContainer(): ()
declare function BeginColumn(style: ContainerStyle?): ()
declare function EndColumn(): ()
declare function BeginRow(style: ContainerStyle?): ()
declare function EndRow(): ()
declare function DrawMesh(mesh: Mesh): ()
declare function RegisterUpdate(callback: () -> ()): ()
declare function Label(text: string, style: LabelStyle?): ()
declare function Image(image: Texture | Mesh, style: ImageStyle?): ()

declare function print(...: any): ()

declare CANVAS_TYPE_SCREEN: number
declare CANVAS_TYPE_WORLD: number

declare ALIGN_NONE: Align
declare ALIGN_TOP: Align
declare ALIGN_LEFT: Align
declare ALIGN_BOTTOM: Align
declare ALIGN_RIGHT: Align
declare ALIGN_TOP_LEFT: Align
declare ALIGN_TOP_RIGHT: Align
declare ALIGN_TOP_CENTER: Align
declare ALIGN_CENTER_LEFT: Align
declare ALIGN_CENTER_RIGHT: Align
declare ALIGN_CENTER: Align
declare ALIGN_BOTTOM_LEFT: Align
declare ALIGN_BOTTOM_RIGHT: Align
declare ALIGN_BOTTOM_CENTER: Align

declare IMAGE_STRETCH_NONE: ImageStretch
declare IMAGE_STRETCH_FILL: ImageStretch
declare IMAGE_STRETCH_UNIFORM: ImageStretch

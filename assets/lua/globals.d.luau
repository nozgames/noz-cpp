-- Type definitions for NoZ Lua API

-- Asset types (opaque userdata)
declare class Font end
declare class Mesh end
declare class Texture end
declare class Sound end
declare class Skeleton end
declare class Animation end
declare class AnimatedMesh end
declare class Shader end
declare class Bin end

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

declare function BeginCanvas(style: CanvasStyle?): ()
declare function EndCanvas(): ()
declare function BeginContainer(style: ContainerStyle?): ()
declare function EndContainer(): ()
declare function DrawMesh(mesh: Mesh): ()
declare function RegisterUpdate(callback: () -> ()): ()
declare function Label(text: string, style: LabelStyle?): ()

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

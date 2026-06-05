import QtQuick

pragma Singleton

QtObject {
    readonly property int barHeight: 26
    readonly property int radius: 6
    readonly property int compactRadius: 4
    readonly property int toolbarButtonSize: 26
    readonly property int toolbarButtonIconSize: 16
    readonly property int toolbarSelectHeight: 26
    readonly property int toolbarSelectDelegateHeight: 24
    readonly property int toolbarSelectWidth: 164
    readonly property int toolbarSelectWideWidth: 218

    readonly property color windowBg: "#1e2028"
    readonly property color panelBg: "#242730"
    readonly property color toolBg: "#252a34"
    readonly property color toolBgHover: "#2b303b"
    readonly property color toolBgActive: "#303542"
    readonly property color stageBg: "#07080c"
    readonly property color border: "#343946"
    readonly property color borderMuted: "#303541"
    readonly property color text: "#d9deea"
    readonly property color mutedText: "#8d95aa"
    readonly property color accent: "#08aeea"
    readonly property color status: "#af8f5d"

    function toolbarButtonBase(color) {
        switch (color) {
        case "primary":
            return "#146ecc"
        case "danger":
            return "#cc3c3f"
        case "warning":
            return "#d55a2c"
        case "success":
            return "#418b36"
        case "secondary":
        default:
            return "#464c5b"
        }
    }

    function toolbarButtonForeground(color) {
        switch (color) {
        case "primary":
            return "#a6deff"
        case "danger":
            return "#ffb7b8"
        case "warning":
            return "#ffc7ac"
        case "success":
            return "#bff8b7"
        case "secondary":
        default:
            return "#c0cfe7"
        }
    }

    function toolbarButtonBg(variant, color) {
        const base = toolbarButtonBase(color)
        if (variant === "ghost" || variant === "icon" || variant === "outline")
            return "transparent"
        return base
    }

    function toolbarButtonHoverBg(variant, color) {
        const base = toolbarButtonBase(color)
        if (variant === "ghost")
            return "transparent"
        if (variant === "icon")
            return Qt.hsla(Qt.color(base).hslHue, Qt.color(base).hslSaturation, Qt.color(base).hslLightness, 0.18)
        if (variant === "outline")
            return Qt.hsla(Qt.color(base).hslHue, Qt.color(base).hslSaturation, Qt.color(base).hslLightness, 0.16)
        return Qt.lighter(base, 1.12)
    }

    function toolbarButtonBorder(variant, color, active, hovered, focused) {
        const base = toolbarButtonBase(color)
        if (variant === "icon")
            return "transparent"
        if (active)
            return base
        if (variant === "ghost")
            return hovered || focused ? "#383b4a" : "transparent"
        if (variant === "outline")
            return hovered || focused ? Qt.lighter(base, 1.2) : base
        return hovered || focused ? Qt.lighter(base, 1.42) : Qt.lighter(base, 1.18)
    }

    function toolbarButtonContent(variant, color, iconOnly) {
        if (variant === "icon" || variant === "outline")
            return Qt.lighter(toolbarButtonBase(color), 1.2)
        if (variant === "ghost")
            return color === "secondary" ? mutedText : toolbarButtonBase(color)
        return toolbarButtonForeground(color)
    }

    function toolbarButtonHoveredContent(variant, color, iconOnly, active, hovered, focused) {
        const normal = toolbarButtonContent(variant, color, iconOnly)
        if ((variant === "ghost" || variant === "icon") && (active || hovered || focused))
            return Qt.lighter(toolbarButtonBase(color), 1.32)
        if (variant === "outline" && (active || hovered || focused))
            return Qt.lighter(toolbarButtonBase(color), 1.32)
        return normal
    }
}

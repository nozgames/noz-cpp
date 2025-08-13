/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#pragma once

namespace noz::ui
{
    class Canvas;
    class Element;
}

namespace noz::node
{
    class Scene;
}

namespace noz::tools
{
    class UIDebugger
    {
    public:
        
        // Print hierarchy of all canvases in a scene
        static void printUIHierarchy(std::shared_ptr<noz::node::Scene> scene);
        
        // Print hierarchy starting from a specific canvas
        static void printCanvasHierarchy(std::shared_ptr<noz::ui::Canvas> canvas, int depth = 0);
        
    private:
        
        // Helper to print element information
        static void printElementInfo(std::shared_ptr<noz::ui::Element> element, int depth);
        
        // Helper to get indentation string
        static std::string getIndent(int depth);
        
        // Helper to get element type name
        static std::string getElementTypeName(std::shared_ptr<noz::ui::Element> element);
        
        // Helper to get applied styles summary
        static std::string getStylesSummary(std::shared_ptr<noz::ui::Element> element);
        
        // Helper to get pseudo state information
        static std::string getPseudoStateInfo(std::shared_ptr<noz::ui::Element> element);
    };
}
/*

	NoZ Game Engine

	Copyright(c) 2025 NoZ Games, LLC

*/

#include <noz/tools/UIDebugger.h>
#include <noz/ui/elements/Canvas.h>
#include <noz/ui/elements/Element.h>
#include <noz/Scene.h>
#include <noz/Node.h>
#include <noz/Log.h>
#include <noz/TypeId.h>
#include <noz/ui/Style.h>

using namespace noz::node;
using namespace noz::ui;

namespace noz::tools
{
    void UIDebugger::printUIHierarchy(std::shared_ptr<noz::node::Scene> scene)
    {
        if (!scene)
        {
            noz::Log::info("UIDebugger", "No scene provided");
            return;
        }
        
        noz::Log::info("UIDebugger", "=== UI Hierarchy Debug ===");
        noz::Log::info("UIDebugger", std::string("Scene: ") + scene->name());
        
        // Find all Canvas nodes in the scene
        std::vector<std::shared_ptr<Canvas>> canvases;
        
        std::function<void(std::shared_ptr<Node>)> findCanvases = [&](std::shared_ptr<Node> node)
        {
            if (!node) return;
            
            // Check if this node is a Canvas
            if (auto canvas = std::dynamic_pointer_cast<Canvas>(node))
            {
                canvases.push_back(canvas);
            }
            
            // Recursively check children
            for (int i = 0; i < node->childCount(); ++i)
            {
                findCanvases(node->child(i));
            }
        };
        
        if (scene->root())
        {
            findCanvases(scene->root());
        }
        
        if (canvases.empty())
        {
            noz::Log::info("UIDebugger", "No Canvas nodes found in scene");
        }
        else
        {
            noz::Log::info("UIDebugger", std::string("Found ") + std::to_string(canvases.size()) + std::string(" canvas(es):"));
            for (auto canvas : canvases)
            {
                printCanvasHierarchy(canvas);
                noz::Log::info("UIDebugger", ""); // Empty line between canvases
            }
        }
        
        noz::Log::info("UIDebugger", "=== End UI Hierarchy Debug ===");
    }
    
    void UIDebugger::printCanvasHierarchy(std::shared_ptr<Canvas> canvas, int depth)
    {
        if (!canvas) return;
        
        noz::Log::info("UIDebugger", getIndent(depth) + "Canvas: '" + canvas->name() + "' [" + getElementTypeName(canvas) + "]");
        noz::Log::info("UIDebugger", getIndent(depth) + "  Sort Order: " + std::to_string(canvas->sortOrder()));
        noz::Log::info("UIDebugger", getIndent(depth) + "  Reference Size: " + std::to_string(canvas->referenceSize().x) + "x" + std::to_string(canvas->referenceSize().y));
        noz::Log::info("UIDebugger", getIndent(depth) + "  Canvas Size: " + std::to_string(canvas->canvasSize().x) + "x" + std::to_string(canvas->canvasSize().y));
        noz::Log::info("UIDebugger", getIndent(depth) + "  Styles: " + getStylesSummary(canvas));
        
        // Print all child elements
        for (int i = 0; i < canvas->childCount(); ++i)
        {
            if (auto childElement = std::dynamic_pointer_cast<Element>(canvas->child(i)))
            {
                printElementInfo(childElement, depth + 1);
            }
        }
    }
    
    void UIDebugger::printElementInfo(std::shared_ptr<Element> element, int depth)
    {
        if (!element) return;
        
        noz::Log::info("UIDebugger", getIndent(depth) + "Element: '" + element->name() + "' [" + getElementTypeName(element) + "]");
        noz::Log::info("UIDebugger", getIndent(depth) + "  Bounds: (" + std::to_string(element->bounds().x) + ", " + std::to_string(element->bounds().y) + ", " + std::to_string(element->bounds().width) + ", " + std::to_string(element->bounds().height) + ")");
        noz::Log::info("UIDebugger", getIndent(depth) + "  Measured Size: " + std::to_string(element->measuredSize().x) + "x" + std::to_string(element->measuredSize().y));
        noz::Log::info("UIDebugger", getIndent(depth) + "  Visible: " + (element->visible() ? "true" : "false"));
        noz::Log::info("UIDebugger", getIndent(depth) + "  Control ID: " + std::to_string(element->controlId()));
        noz::Log::info("UIDebugger", getIndent(depth) + "  Pseudo State: " + getPseudoStateInfo(element));
        noz::Log::info("UIDebugger", getIndent(depth) + "  Styles: " + getStylesSummary(element));
        
        // Recursively print children
        for (int i = 0; i < element->childCount(); ++i)
        {
            if (auto childElement = std::dynamic_pointer_cast<Element>(element->child(i)))
            {
                printElementInfo(childElement, depth + 1);
            }
        }
    }
    
    std::string UIDebugger::getIndent(int depth)
    {
        std::string indent = "";
        for (int i = 0; i < depth; ++i)
        {
            indent += "  ";
        }
        return indent;
    }
    
    std::string UIDebugger::getElementTypeName(std::shared_ptr<Element> element)
    {
        if (!element) return "null";
        
        // Use the TypeId system to get the actual type name
        return element->typeId().name();
    }
    
    std::string UIDebugger::getStylesSummary(std::shared_ptr<Element> element)
    {
        if (!element) return "none";
        
        const auto& style = element->style();
        std::string summary = "";
        
        // Add key style properties
        if (style.backgroundColor.value != noz::Color::Transparent)
        {
            summary += "bg-color ";
        }
        
        if (!style.width.isAuto())
        {
            summary += "width ";
        }
        
        if (!style.height.isAuto())
        {
            summary += "height ";
        }
        
        if ((style.marginLeft.unit != StyleLength::Unit::Fixed || style.marginLeft.value != 0.0f) ||
            (style.marginTop.unit != StyleLength::Unit::Fixed || style.marginTop.value != 0.0f) ||
            (style.marginRight.unit != StyleLength::Unit::Fixed || style.marginRight.value != 0.0f) ||
            (style.marginBottom.unit != StyleLength::Unit::Fixed || style.marginBottom.value != 0.0f))
        {
            summary += "margin ";
        }
        
        if ((style.paddingLeft.unit != StyleLength::Unit::Fixed || style.paddingLeft.value != 0.0f) ||
            (style.paddingTop.unit != StyleLength::Unit::Fixed || style.paddingTop.value != 0.0f) ||
            (style.paddingRight.unit != StyleLength::Unit::Fixed || style.paddingRight.value != 0.0f) ||
            (style.paddingBottom.unit != StyleLength::Unit::Fixed || style.paddingBottom.value != 0.0f))
        {
            summary += "padding ";
        }
        
        return summary.empty() ? "none" : summary;
    }
    
    std::string UIDebugger::getPseudoStateInfo(std::shared_ptr<Element> element)
    {
        if (!element) return "none";
        
        PseudoState directState = element->pseudoState();
        PseudoState effectiveState = element->effectivePseudoState();
        
        std::string directStateStr = pseudoStateToString(directState);
        std::string effectiveStateStr = pseudoStateToString(effectiveState);
        
        if (directStateStr.empty()) directStateStr = "none";
        if (effectiveStateStr.empty()) effectiveStateStr = "none";
        
        // If direct and effective are the same, just show one
        if (directState == effectiveState)
        {
            return directStateStr;
        }
        else
        {
            // Show both: direct (inherited from parent)
            return directStateStr + " (effective: " + effectiveStateStr + ")";
        }
    }
}
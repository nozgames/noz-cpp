///////////////////////////////////////////////////////////////////////////////
// NoZ Engine Framework
// Copyright (C) 2015 NoZ Games, LLC
// http://www.nozgames.com
///////////////////////////////////////////////////////////////////////////////

#include <noz.pch.h>
#include <noz/Nodes/UI/ListViewItem.h>
#include "ConsoleView.h"

using namespace noz;
using namespace noz::Editor;

NOZ_TODO("warning, info, and error filter buttons");
NOZ_TODO("clear button");
NOZ_TODO("collapse button?");
NOZ_TODO("warning, info, and error icons (color code lines) (use style for each ConsoleViewErrorItem, etc)");

ConsoleView::ConsoleView(void) {
}

ConsoleView::~ConsoleView(void) {
}

bool ConsoleView::OnApplyStyle (void) {
  if(!Control::OnApplyStyle()) return false;
  if(nullptr == list_view_) return false;

  // To ensure that the loading of the ListViewItem for the console text does not generate 
  // an infinite recursion by outputing console text during the load we disconnect from the console
  // and then force a load of the current list view item style and then reconnect to the console
  Console::ConsoleMessageWritten -= ConsoleMessageEventHandler::Delegate(this,&ConsoleView::OnConsoleMessage);

  ListViewItem* item = new ListViewItem;
  list_view_->AddChild(item);
  list_view_->RemoveChildAt(0);

  Console::ConsoleMessageWritten += ConsoleMessageEventHandler::Delegate(this,&ConsoleView::OnConsoleMessage);

  return true;
}

void ConsoleView::OnConsoleMessage (ConsoleMessageType type, Object* context, const char* msg) {
  if(nullptr == list_view_) return;
  ListViewItem* item = new ListViewItem;
  item->SetText(msg);
  list_view_->AddChild(item);
  list_view_->UnselectAll();
  item->SetSelected(true);
  item->BringIntoView();
}

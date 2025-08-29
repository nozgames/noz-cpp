# NoZ Editor

## Implementation

- TUI (Text User Interface) application
- default view is a log view over whole screen except for bottom two rows
- bottom row is blank until you hit : (like vim) and then you can type commands
- second to last row is full white and reserved as a status bar
- Main view which is a log is just one view, can replace the view with other views like a entity viewer
- importer log messages go to the main log view
- :q to quit

// Waveshare ESP32-S3-Touch-AMOLED-1.75" Display Module
// Imported from STEP file via FreeCAD conversion
//
// For dead-front enclosure design: the display sits behind a
// translucent/diffused panel, with only the screen content visible
// through light-passing areas (hidden-until-lit effect)

module display_module() {
    import("ESP32-S3-Touch-AMOLED-1_75.stl");
}

// Display dimensions from Waveshare spec:
// - 1.75" AMOLED diagonal
// - 466 x 466 pixels
// - Module dimensions: ~46mm x 46mm x 5.7mm (including touch)

// Show the imported module for reference
display_module();

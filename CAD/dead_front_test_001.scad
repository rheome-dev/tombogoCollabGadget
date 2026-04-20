// Dead Front Test Piece - Version 001
// For testing hidden-until-lit display technique
//
// Design: Simple front face plate with thin translucent window,
// open back for easy module insertion. Can use circular or square cutout.

// ============================================================================
// CONFIGURATION
// ============================================================================

// Show the Waveshare module STL for alignment verification
SHOW_MODULE = true;       // Set to false before exporting STL

// Module dimensions (from STL analysis)
MODULE_DIAMETER = 49;     // Circular module diameter
MODULE_THICKNESS = 13;    // Y-axis depth

// Fit tolerance - CHANGE THIS to adjust snugness
// 0.3 = snug fit, 0.5 = easy fit, 1.0 = loose
FIT_CLEARANCE = 1;      // Total clearance (0.2mm per side)

// Face plate dimensions
FACE_WIDTH = 58;          // Slightly larger than module
FACE_DEPTH = 58;          // Square
FACE_THICKNESS = 14;      // Total thickness

// Dead front window (thin section for display to shine through)
WINDOW_THICKNESS = 0.6;   // 0.6-1.0mm for touch sensitivity

// Cutout shape: "square" or "circular"
CUTOUT_SHAPE = "circular";  // CHANGE THIS: "square" or "circular"

// Calculated cutout size (derived from module + clearance)
CUTOUT_DIAMETER = MODULE_DIAMETER + FIT_CLEARANCE;

// USB-C cable opening (now centered on face)
CABLE_WIDTH = 14;
CABLE_HEIGHT = 8;

// ============================================================================
// MODULES
// ============================================================================

module face_plate() {
    // Simple front face plate - open back
    cube([FACE_WIDTH, FACE_THICKNESS, FACE_DEPTH]);
}

module module_cavity_square() {
    // Square cavity that stops at WINDOW_THICKNESS from the front face
    cavity_size = MODULE_DIAMETER + FIT_CLEARANCE;
    translate([
        (FACE_WIDTH - cavity_size) / 2,
        -1,  // Start from back (open)
        (FACE_DEPTH - cavity_size) / 2
    ]) {
        cube([
            cavity_size,
            FACE_THICKNESS - WINDOW_THICKNESS + 1,  // Stop at window thickness
            cavity_size
        ]);
    }
}

module module_cavity_circular() {
    // Circular cutout that stops at WINDOW_THICKNESS from the front face
    // This creates a continuous thin window layer
    // Start at Y=-1 to cleanly cut through back face (avoids z-fighting)
    translate([FACE_WIDTH / 2, -1, FACE_DEPTH / 2]) {
        rotate([-90, 0, 0]) {
            cylinder(
                h = FACE_THICKNESS - WINDOW_THICKNESS + 1,  // +1 to compensate for -1 start
                r = CUTOUT_DIAMETER / 2,
                $fn = 64
            );
        }
    }
}

module cable_opening() {
    // USB-C cable opening - on bottom edge only
    // Cuts from back toward front, but STOPS before the window layer
    // This keeps the dead front face intact

    cable_depth = FACE_THICKNESS - WINDOW_THICKNESS - 1;  // Stop 1mm before window

    translate([
        (FACE_WIDTH - CABLE_WIDTH) / 2,  // Centered on X
        -1,                               // Start from back (open side)
        -1                                // Bottom edge (Z=0)
    ]) {
        cube([CABLE_WIDTH, cable_depth + 1, CABLE_HEIGHT + 2]);
    }
}

module dead_front_test() {
    difference() {
        // Main body
        face_plate();

        // Module cavity - creates the thin window naturally by not cutting all the way through
        if (CUTOUT_SHAPE == "circular") {
            module_cavity_circular();
        } else {
            module_cavity_square();
        }

        // Cable opening (centered)
        cable_opening();
    }
}

module waveshare_module() {
    // Import the actual Waveshare ESP32-S3-Touch-AMOLED-1.75 STL
    // Positioned to sit inside the cavity for alignment verification
    //
    // ADJUST THESE VALUES to align the module with the enclosure:
    MODULE_X_OFFSET = 29;    // Left/Right (increase = move right)
    MODULE_Y_OFFSET = 10;    // Back/Front (increase = move toward front/window)
    MODULE_Z_OFFSET = 29;    // Down/Up (increase = move up)
    MODULE_ROTATION = 270;    // Rotation around Y axis (vertical)

    translate([MODULE_X_OFFSET, MODULE_Y_OFFSET, MODULE_Z_OFFSET]) {
        rotate([0, MODULE_ROTATION, 0]) {
            import("ESP32-S3-Touch-AMOLED-1_75_fixed.stl", convexity=10);
        }
    }
}

// ============================================================================
// RENDER
// ============================================================================

dead_front_test();

// Show the actual Waveshare module STL for alignment verification
// Uses % for transparent preview (won't be included in STL export)
if (SHOW_MODULE) {
    %color("DarkGreen", 0.6)
    waveshare_module();
}

// ============================================================================
// USAGE NOTES
// ============================================================================
//
// To see changes after editing parameters:
//   - Press F5 for quick preview
//   - Press F6 for full render (slower but accurate)
//
// SHOW_MODULE = true:
//   - Shows the actual Waveshare STL in transparent green
//   - Verify alignment and tolerances before printing
//   - The % prefix means it won't be included in STL export
//   - Set to false before final export to speed up rendering
//
// WINDOW_THICKNESS controls the dead front layer:
//   - 0.6mm = more light transmission, less touch sensitivity
//   - 1.0mm = balanced
//   - 1.2mm = better durability, may reduce touch sensitivity
//
// If module position looks wrong:
//   - Adjust MODULE_X/Y/Z_OFFSET in waveshare_module()
//   - The STL origin point may differ from expected
//
// Print in natural/translucent PLA or PETG
// Sand the INSIDE of the window for diffusion effect
//

# Fusion 360 API script
# Sketch on XY plane offset +3 mm in Z; spline through y=x^2 (10 pts, x in [-2, 2] mm),
# then a line joining the spline endpoints.

import adsk.core, adsk.fusion, adsk.cam, traceback, math

MM_TO_CM = 0.1  # Fusion API internal units are centimeters

def deg(v): return v * math.pi / 180.0  # degrees -> radians

def make_rotation(axis_vec3d, angle_deg, pivot_pt):
    m = adsk.core.Matrix3D.create()
    m.setToRotation(deg(angle_deg), axis_vec3d, pivot_pt)
    return m

theta_degrees = 10  # tilt in x (radians)

def reposition_T():
    """
    Build M = Rx(alpha) -> Ry(beta) -> Rz(gamma) -> T(a,b,c)
    (i.e., apply Rx, then Ry, then Rz, then translate).
    """
    a, b, c = 1.2656, 1.8603, .6605          # translate by (a,b,c)
    alpha, beta, gamma = -90, 180 + theta_degrees, -90.0  # rotate X,Y,Z in degrees
    pivot = adsk.core.Point3D.create(0,0,0)  # rotate about global origin

    # Start with Rx
    M = make_rotation(adsk.core.Vector3D.create(1,0,0), alpha, pivot)
    # Then Ry
    M.transformBy(make_rotation(adsk.core.Vector3D.create(0,1,0), beta, pivot))
    # Then Rz
    M.transformBy(make_rotation(adsk.core.Vector3D.create(0,0,1), gamma, pivot))
    # Finally the translation
    T = adsk.core.Matrix3D.create()
    T.translation = adsk.core.Vector3D.create(a, b, c)  # sets the translation part
    M.transformBy(T)
    return M

# Lens parameters
f_x = 3.5  # mm
f_z = 5.2  # mm
k_x = -1.0
k_z = -1.0
y_v = -1.5  # mm
y_min = -3.25  # mm, minimum y value to avoid NaNs
n = 1.507
c_x = n / (f_x * (n - 1))
c_z = n / (f_z * (n - 1))
def lens_y(x, z):
    factor = 1 - (1 + k_x) * c_x**2 * x**2 - (1 + k_z) * c_z**2 * z**2
    if factor < 0:
        return y_min
    denom = 1 + math.sqrt(factor)
    sag = (c_x * x**2 + c_z * z**2) / denom
    return max(y_min, y_v - sag)

# TIRM parameters
x_f = 17.4  # mm
y_f = -6.575  # mm
f = 8.5  # mm
def tirm_y(x, z):
    return y_f - f + ((x - x_f)**2 + z**2) / (4 * f)

def create_lens_sketch(root):
    # --- 1) Construction plane offset from XY at Z=0 ---
    planes = root.constructionPlanes
    pl_in  = planes.createInput()
    offset_val = adsk.core.ValueInput.createByReal(0)  # 0 mm -> 0 cm
    pl_in.setByOffset(root.xYConstructionPlane, offset_val)
    cplane = planes.add(pl_in)

    # --- 2) New sketch on that plane ---
    sketches = root.sketches
    sk = sketches.add(cplane)

    # --- 3) Build points for y = lens(x, 0) (10 points, x ∈ [-2, 2] mm) ---
    n_pts = 20
    x_min_mm, x_max_mm = 0, 1.25
    if n_pts < 2:
        raise ValueError("Need at least 2 points for a spline.")
    step = (x_max_mm - x_min_mm) / (n_pts - 1)

    pts = adsk.core.ObjectCollection.create()
    xs_mm = [x_min_mm + i*step for i in range(n_pts)]
    for x_mm in xs_mm:
        y_mm = lens_y(x_mm, 0)  # y = lens(x, 0) (mm)
        # In sketch space, z=0 because sketch lies on the construction plane
        pts.add(adsk.core.Point3D.create(x_mm * MM_TO_CM, y_mm * MM_TO_CM, 0.0))

    # --- 4) Create a fitted spline through the points ---
    spline = sk.sketchCurves.sketchFittedSplines.add(pts)

    # --- 5) Foot points on the x-axis (y = 0) with SAME x as endpoints ----
    start_sp = spline.startSketchPoint
    end_sp   = spline.endSketchPoint

    # ---- Foot points on the x-axis (y = 0) with SAME x as endpoints ----
    # (The prompt said "same y coordinate", but to hit the x-axis the y must be 0.
    #  Typically the intent is dropping verticals: same x, y=0.)
    cartridge_top = -3.25 * MM_TO_CM  # mm
    start_x = start_sp.geometry.x
    end_x   = end_sp.geometry.x
    foot_start = adsk.core.Point3D.create(start_x, cartridge_top, 0.0)
    foot_end   = adsk.core.Point3D.create(end_x,   cartridge_top, 0.0)

    # ---- Draw the two vertical lines from endpoints to the x-axis ----
    lines = sk.sketchCurves.sketchLines
    lines.addByTwoPoints(start_sp, foot_start)
    lines.addByTwoPoints(end_sp,   foot_end)

    # ---- Draw the base line along the x-axis joining the two feet ----
    lines.addByTwoPoints(foot_start, foot_end)

    # Optional: make axes visible in the sketch for clarity
    sk.isAxesVisible = True
    
    # Optional: name the sketch for clarity
    sk.name = "y = lens(x, 0)"
    cplane.deleteMe()  # Clean up construction plane
    return sk

def create_tirm_sketch(root, z_mm):
    # --- 1) Construction plane offset from XY at Z=0 ---
    planes = root.constructionPlanes
    pl_in  = planes.createInput()
    offset_val = adsk.core.ValueInput.createByReal(z_mm * MM_TO_CM)  # mm -> cm
    pl_in.setByOffset(root.xYConstructionPlane, offset_val)
    cplane = planes.add(pl_in)

    # --- 2) New sketch on that plane ---
    sketches = root.sketches
    sk = sketches.add(cplane)

    # --- 3) Build points for y = lens(x, 0) (10 points, x ∈ [-2, 2] mm) ---
    n_pts = 20
    x_min_mm, x_max_mm = -1.25, 1.25
    y_mm_min = -100
    if n_pts < 2:
        raise ValueError("Need at least 2 points for a spline.")
    step = (x_max_mm - x_min_mm) / (n_pts - 1)

    pts = adsk.core.ObjectCollection.create()
    xs_mm = [x_min_mm + i*step for i in range(n_pts)]
    for x_mm in xs_mm:
        y_mm = tirm_y(x_mm, z_mm)  # y = tirm(x, z) (mm)
        # In sketch space, z=0 because sketch lies on the construction plane
        if math.isnan(y_mm):
            continue
        if y_mm < y_mm_min:
            y_mm = y_mm_min
        pts.add(adsk.core.Point3D.create(x_mm * MM_TO_CM, y_mm * MM_TO_CM, 0.0))

    # --- 4) Create a fitted spline through the points ---
    spline = sk.sketchCurves.sketchFittedSplines.add(pts)

    # --- 5) Foot points on the x-axis (y = 0) with SAME x as endpoints ----
    start_sp = spline.startSketchPoint
    end_sp   = spline.endSketchPoint

    # ---- Foot points on the x-axis (y = 0) with SAME x as endpoints ----
    #  Typically the intent is dropping verticals: same x, y=0.)
    cartridge_top = -4 * MM_TO_CM  # mm
    start_x = start_sp.geometry.x
    end_x   = end_sp.geometry.x
    foot_start = adsk.core.Point3D.create(start_x, cartridge_top, 0.0)
    foot_end   = adsk.core.Point3D.create(end_x,   cartridge_top, 0.0)

    # ---- Draw the two vertical lines from endpoints to the x-axis ----
    lines = sk.sketchCurves.sketchLines
    lines.addByTwoPoints(start_sp, foot_start)
    lines.addByTwoPoints(end_sp,   foot_end)

    # ---- Draw the base line along the x-axis joining the two feet ----
    lines.addByTwoPoints(foot_start, foot_end)

    # Optional: make axes visible in the sketch for clarity
    sk.isAxesVisible = True
    sk.isVisible = False  # Hide the sketch to reduce visual clutter
    
    # Optional: name the sketch for clarity
    sk.name = "y = lens(x, 0)"
    cplane.deleteMe()  # Clean up construction plane
    return sk

def run(context):
    app = adsk.core.Application.get()
    ui  = app.userInterface
    try:
        # Get the active design
        design = adsk.fusion.Design.cast(app.activeProduct)
        if not design:
            raise RuntimeError("No active Fusion design.")

        root = design.rootComponent
        T = reposition_T()  # Move bodies after creation

        # Create the lens
        lens_sketch = create_lens_sketch(root)
        lens_profile = lens_sketch.profiles.item(0)

        # --- Create a 360° revolve about the global Y axis ---
        revolves = root.features.revolveFeatures
        # New Body result
        revInput = revolves.createInput(
            lens_profile,
            root.yConstructionAxis,  # revolve axis
            adsk.fusion.FeatureOperations.NewBodyFeatureOperation
        )
        # Angle: 360 degrees
        angle = adsk.core.ValueInput.createByString('360 deg')
        revInput.setAngleExtent(False, angle)  # False = one-side extent
        # (revInput.isSolid is True by default for closed profiles, but it's okay to set explicitly)
        revInput.isSolid = True

        lens = revolves.add(revInput)
        lens_sketch.deleteMe()  # Clean up the sketch

        sel = adsk.core.ObjectCollection.create()
        sel.add(lens.bodies.item(0))

        moveFeats = root.features.moveFeatures
        moveInput = moveFeats.createInput(sel, T) 
        moveFeats.add(moveInput)

        # Create the TIRM
        n_sketches = 20
        tirm_sketches = []
        z_min_mm, z_max_mm = -2.0, 2.0
        if n_sketches < 2:
            raise ValueError("Need at least 2 sketches for the TIRM.")
        step = (z_max_mm - z_min_mm) / (n_sketches - 1)

        sections = adsk.core.ObjectCollection.create()

        zs_mm = [z_min_mm + i*step for i in range(n_sketches)]
        for z_mm in zs_mm:
            sk = create_tirm_sketch(root, z_mm)
            sections.add(sk.profiles.item(0))  # take the first closed region
            tirm_sketches.append(sk)

        # Create TIRM as a solid, New Body
        loftFeats = root.features.loftFeatures
        loftInput = loftFeats.createInput(adsk.fusion.FeatureOperations.NewBodyFeatureOperation)

        # Add the three section profiles in order
        for i in range(sections.count):
            loftInput.loftSections.add(sections.item(i))

        loftInput.isSolid = True              # important: make a solid
        # Optional: continuity settings, etc., can be adjusted here if needed
        # loftInput.sectionCoedgeContinuity = adsk.fusion.SurfaceContinuityTypes.SurfaceContinuityTypeNone

        tirm = loftFeats.add(loftInput)

        sel = adsk.core.ObjectCollection.create()
        sel.add(tirm.bodies.item(0))

        moveFeats = root.features.moveFeatures
        moveInput = moveFeats.createInput(sel, T) 
        moveFeats.add(moveInput)
        
        for sk in tirm_sketches:
            sk.deleteMe()  # Clean up the sketches

        # Create lens sketch to be rotated later
        if ui:
            ui.messageBox("Optical surfaces created.")

    except:
        if ui:
            ui.messageBox('Failed:\n{}'.format(traceback.format_exc()))

def stop(context):
    # Nothing to clean up explicitly
    pass
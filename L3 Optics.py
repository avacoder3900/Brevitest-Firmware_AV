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

target_x, target_y, target_z = -0.5361, 1.8609, 0.5105          # translate by (a,b,c)

def reposition_T(theta=0, x=0, y=0, z=0):
    """
    Build M = Rx(theta) -> T(a,b,c)
    (i.e., apply Rx, then Ry, then Rz, then translate).
    """
    pivot = adsk.core.Point3D.create(0,0,0)  # rotate about global origin

    # Start with Rx
    M = make_rotation(adsk.core.Vector3D.create(1,0,0), theta, pivot)
    # Finally the translation
    T = adsk.core.Matrix3D.create()
    T.translation = adsk.core.Vector3D.create(x, y, z)  # sets the translation part
    M.transformBy(T)
    return M

def asphere_sag(r, c, k, a2 = 0, a4 = 0):
    """
    Compute the sag (z) for an aspheric surface given radial distance r.
    
    Parameters:
    r (float or np.array): Radial distance sqrt(x^2 + y^2) in mm
    c (float): Curvature (1/radius) in mm^-1
    k (float): Conic constant
    
    Returns:
    float or np.array: Sag z in mm
    """
    return (c * r**2) / (1 + math.sqrt(1 - (1 + k) * c**2 * r**2)) + a2 * r**4 + a4 * r**6

theta  = deg(14.5) # rotation (deg to radians)
lens_radius_mm = 2.5  # mm
lens_base = -3.25  # mm (base of lens, z at r=0)
def lens_surface_optimal(x, y):
    # Optimal parameters
    c      = 0.28       # curvature [1/mm]
    k      = -2.4       # conic constant
    A2     = 0.00015    # [mm^-3]
    A4     = -0.000008  # [mm^-5]
    z0     = -2.65      # vertex z [mm]

    # Rotation around x-axis for y coordinate
    y_rot = y * math.cos(theta) - (z0 + 1) * math.sin(theta)
    r_sq = x**2 + y_rot**2

    # Aspheric sag equation
    discrim = 1 - (1 + k) * c**2 * r_sq
    if discrim < 0:
        return None  # Outside valid region
    z_sag = (c * r_sq) / (1 + math.sqrt(discrim))
    z_sag += A2 * r_sq**2 + A4 * r_sq**3
    return z_sag + z0

def create_lens_sketch(root, x_mm=0):
    # --- 1) Construction plane offset from XY at Z=0 ---
    planes = root.constructionPlanes
    pl_in  = planes.createInput()
    offset_val = adsk.core.ValueInput.createByReal(x_mm * MM_TO_CM)  # 0 mm -> 0 cm
    pl_in.setByOffset(root.xYConstructionPlane, offset_val)
    cplane = planes.add(pl_in)

    # --- 2) New sketch on that plane ---
    sketches = root.sketches
    sk = sketches.add(cplane)

    # --- 3) Build points for y = lens(x) (10 points, x ∈ [-2, 2] mm) ---
    n_pts = 20
    if n_pts < 2:
        raise ValueError("Need at least 2 points for a spline.")
    y_mm_max = lens_radius_mm * math.sin(math.acos(x_mm / lens_radius_mm))
    y_mm_min = -y_mm_max
    step = (y_mm_max - y_mm_min) / (n_pts - 1)

    pts = adsk.core.ObjectCollection.create()
    ys_mm = [y_mm_min + i*step for i in range(n_pts)]
    for y_mm in ys_mm:
        z = lens_surface_optimal(x_mm, y_mm)  # z = lens(x, y) (mm)
        pts.add(adsk.core.Point3D.create(y_mm * MM_TO_CM, z * MM_TO_CM, 0))

    # --- 4) Create a fitted spline through the points ---
    spline = sk.sketchCurves.sketchFittedSplines.add(pts)

    # --- 5) Foot points on the x-axis (y = 0) with SAME x as endpoints ----
    start_sp = spline.startSketchPoint
    end_sp   = spline.endSketchPoint

    # # ---- Foot points on the x-axis (y = 0) with SAME x as endpoints ----
    # #  Typically the intent is dropping verticals: same y, y=0.)
    # start_y = start_sp.geometry.y
    # end_y  = end_sp.geometry.y
    # foot_start = adsk.core.Point3D.create(lens_base * MM_TO_CM, start_y, 0.0)
    # foot_end   = adsk.core.Point3D.create(lens_base * MM_TO_CM, end_y, 0.0)

    # # ---- Draw the two vertical lines from endpoints to the x-axis ----
    # lines = sk.sketchCurves.sketchLines
    # lines.addByTwoPoints(start_sp, foot_start)
    # lines.addByTwoPoints(end_sp,   foot_end)

    # # # ---- Draw the base line along the x-axis joining the two feet ----
    # lines.addByTwoPoints(foot_start, foot_end)

    # Optional: make axes visible in the sketch for clarity
    sk.isAxesVisible = True
    
    # Optional: name the sketch for clarity
    sk.name = "y = lens(x, 0)"
    cplane.deleteMe()  # Clean up construction plane
    return sk

# Parameters for TIRM (from optimization)
c_t = 0.2  # mm^-1
k_t = -1.0
theta_t = 45.0  # degrees
v_y_t = -5.0  # mm (vertex y-position)
v_z_t = -7.0  # mm (vertex z-position)
def tirm_z(x=0, y=0):
    r = math.sqrt(x**2 + y**2)
    sag = asphere_sag(r, c_t, k_t)
    return v_z_t + sag

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
        y_mm = tirm_z(x_mm, z_mm)  # y = tirm(x, z) (mm)
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

        # Create the lens
        
        n_pts = 2
        x_mm_max = lens_radius_mm
        x_mm_min = -lens_radius_mm
        step = (x_mm_max - x_mm_min) / (n_pts - 1)

        lens_sketches = []
        xs_mm = [x_mm_min + i*step for i in range(n_pts)]
        for x_mm in xs_mm:
            lens_sketch = create_lens_sketch(root, x_mm)
            lens_sketches.append(lens_sketch)

        # lens_profile = lens_sketch.profiles.item(0)

        # # # --- Create a 360° revolve about the global Y axis ---
        # revolves = root.features.revolveFeatures
        # # # New Body result
        # revInput = revolves.createInput(
        #     lens_profile,
        #     root.zConstructionAxis,  # revolve axis
        #     adsk.fusion.FeatureOperations.NewBodyFeatureOperation
        # )
        # # # Angle: 360 degrees
        # angle = adsk.core.ValueInput.createByString('360 deg')
        # revInput.setAngleExtent(False, angle)  # False = one-side extent
        # # # (revInput.isSolid is True by default for closed profiles, but it's okay to set explicitly)
        # revInput.isSolid = True

        # lens = revolves.add(revInput)
        # lens_sketch.deleteMe()  # Clean up the sketch

        # sel = adsk.core.ObjectCollection.create()
        # sel.add(lens.bodies.item(0))

        # # Rotate the lens to point toward the target
        # pivot = adsk.core.Point3D.create(v_z_l, 0, 0)  # rotate about global origin
        # Rx = make_rotation(adsk.core.Vector3D.create(1,0,0), theta_l, pivot)
        # T = adsk.core.Matrix3D.create()
        # T.translation = adsk.core.Vector3D.create(target_x, target_y, target_z - v_z_l * MM_TO_CM)  # sets the translation part
        # Rx.transformBy(T)
        # moveFeats = root.features.moveFeatures
        # moveInput = moveFeats.createInput(sel, Rx) 
        # moveFeats.add(moveInput)

        # # Create the TIRM
        # n_sketches = 20
        # tirm_sketches = []
        # z_min_mm, z_max_mm = -2.0, 2.0
        # if n_sketches < 2:
        #     raise ValueError("Need at least 2 sketches for the TIRM.")
        # step = (z_max_mm - z_min_mm) / (n_sketches - 1)

        # sections = adsk.core.ObjectCollection.create()

        # zs_mm = [z_min_mm + i*step for i in range(n_sketches)]
        # for z_mm in zs_mm:
        #     sk = create_tirm_sketch(root, z_mm)
        #     sections.add(sk.profiles.item(0))  # take the first closed region
        #     tirm_sketches.append(sk)

        # # Create TIRM as a solid, New Body
        # loftFeats = root.features.loftFeatures
        # loftInput = loftFeats.createInput(adsk.fusion.FeatureOperations.NewBodyFeatureOperation)

        # # Add the three section profiles in order
        # for i in range(sections.count):
        #     loftInput.loftSections.add(sections.item(i))

        # loftInput.isSolid = True              # important: make a solid
        # # Optional: continuity settings, etc., can be adjusted here if needed
        # # loftInput.sectionCoedgeContinuity = adsk.fusion.SurfaceContinuityTypes.SurfaceContinuityTypeNone

        # tirm = loftFeats.add(loftInput)

        # sel = adsk.core.ObjectCollection.create()
        # sel.add(tirm.bodies.item(0))

        # T = reposition_T(theta_t, v_y_t, v_z_t)  # Move bodies after creation
        # moveFeats = root.features.moveFeatures
        # moveInput = moveFeats.createInput(sel, T) 
        # moveFeats.add(moveInput)
        
        # for sk in tirm_sketches:
        #     sk.deleteMe()  # Clean up the sketches

        # if ui:
        #     ui.messageBox("Optical surfaces created.")

    except:
        if ui:
            ui.messageBox('Failed:\n{}'.format(traceback.format_exc()))

def stop(context):
    # Nothing to clean up explicitly
    pass
"""Coordinate conversion between Blender and the AvP engine.

Blender: right-handed, Z up, metres; characters face -Y (front view).
AvP:     right-handed, x right, y DOWN, z forward, integer millimetres.

    avp = A * blender * SCALE      with A = [[-1,0,0],[0,0,-1],[0,-1,0]]

A is a proper rotation (det +1) and its own inverse, so a rotation matrix R
converts as A R A in either direction and face winding / normals carry over
unchanged.
"""

from mathutils import Matrix, Quaternion, Vector

SCALE = 1000.0  # AvP units (mm) per Blender unit (m)

A3 = Matrix(((-1.0, 0.0, 0.0), (0.0, 0.0, -1.0), (0.0, -1.0, 0.0)))


def vec_to_avp(v, scale=SCALE):
    w = A3 @ Vector(v)
    return [int(round(w.x * scale)), int(round(w.y * scale)), int(round(w.z * scale))]


def vec_to_avp_f(v):
    w = A3 @ Vector(v)
    return [w.x, w.y, w.z]


def vec_from_avp(v, scale=SCALE):
    return A3 @ Vector((v[0] / scale, v[1] / scale, v[2] / scale))


def rot_to_avp(r3):
    return A3 @ r3 @ A3


def rot_from_avp(r3):
    return A3 @ r3 @ A3


def quat_from_file(q):
    """File quaternion (x, y, z, w) -> mathutils rotation matrix in AvP space."""
    return Quaternion((q[3], q[0], q[1], q[2])).to_matrix()


def quat_to_file(r3):
    q = r3.to_quaternion().normalized()
    return [q.x, q.y, q.z, q.w]


def matrix_from_avp(r3_avp, t_avp, scale=SCALE):
    """AvP (rotation, translation mm) -> Blender 4x4."""
    m = rot_from_avp(r3_avp).to_4x4()
    m.translation = vec_from_avp(t_avp, scale)
    return m


def matrix_to_avp(m4, scale=SCALE):
    """Blender 4x4 (rigid) -> (AvP rotation 3x3, translation mm floats)."""
    loc, rot, _ = m4.decompose()
    r = rot_to_avp(rot.to_matrix())
    t = A3 @ loc
    return r, [t.x * scale, t.y * scale, t.z * scale]

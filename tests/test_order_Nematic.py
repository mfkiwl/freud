import numpy as np
import numpy.testing as npt
import freud
import rowan
import unittest


class TestNematicOrder(unittest.TestCase):
    def test_perfect(self):
        """Test perfectly aligned systems with different molecular axes"""
        N = 1000
        axes = np.zeros(shape=(N, 3), dtype=np.float32)
        angles = np.zeros(shape=N, dtype=np.float32)
        axes[:, 0] = 1.0
        orientations = rowan.from_axis_angle(axes, angles)

        # Test for parallel to molecular axis
        u = np.array([1, 0, 0])
        op_parallel = freud.order.NematicOrderParameter(u)

        # Test access
        with self.assertRaises(AttributeError):
            op_parallel.nematic_order_parameter
        with self.assertRaises(AttributeError):
            op_parallel.director
        with self.assertRaises(AttributeError):
            op_parallel.particle_tensor
        with self.assertRaises(AttributeError):
            op_parallel.nematic_tensor

        op_parallel.compute(orientations)

        # Test access
        op_parallel.nematic_order_parameter
        op_parallel.director
        op_parallel.particle_tensor
        op_parallel.nematic_tensor

        self.assertTrue(op_parallel.nematic_order_parameter == 1)
        npt.assert_equal(op_parallel.director, u)
        npt.assert_equal(
            op_parallel.nematic_tensor, np.diag([1, -0.5, -0.5]))
        npt.assert_equal(
            op_parallel.nematic_tensor, np.mean(
                op_parallel.particle_tensor, axis=0))

        # Test for perpendicular to molecular axis
        u = np.array([0, 1, 0])
        op_perp = freud.order.NematicOrderParameter(u)
        op_perp.compute(orientations)

        self.assertEqual(op_perp.nematic_order_parameter, 1)
        npt.assert_equal(op_perp.director, u)
        npt.assert_equal(
            op_perp.nematic_tensor, np.diag([-0.5, 1, -0.5]))

    def test_imperfect(self):
        """Test imperfectly aligned systems.
        We add some noise to the perfect system and see if the output is close
        to the ideal case.
        """
        N = 1000
        np.random.seed(0)

        # Generate orientations close to the identity quaternion
        orientations = \
            rowan.interpolate.slerp([1, 0, 0, 0], rowan.random.rand(N), 0.1)

        u = np.array([1, 0, 0])
        op = freud.order.NematicOrderParameter(u)
        op.compute(orientations)

        npt.assert_allclose(op.nematic_order_parameter, 1, atol=1e-1)
        self.assertNotEqual(op.nematic_order_parameter, 1)

        npt.assert_allclose(op.director, u, atol=1e-1)
        self.assertFalse(np.all(op.director == u))

        npt.assert_allclose(
            op.nematic_tensor, np.diag([1, -0.5, -0.5]), atol=1e-1)
        self.assertFalse(np.all(op.nematic_tensor == np.diag([1, -0.5, -0.5])))

        u = np.array([0, 1, 0])
        op_perp = freud.order.NematicOrderParameter(u)
        op_perp.compute(orientations)

        npt.assert_allclose(op_perp.nematic_order_parameter, 1, atol=1e-1)
        self.assertNotEqual(op_perp.nematic_order_parameter, 1)

        npt.assert_allclose(op_perp.director, u, atol=1e-1)
        self.assertFalse(np.all(op_perp.director == u))

        npt.assert_allclose(
            op_perp.nematic_tensor, np.diag([-0.5, 1, -0.5]), atol=1e-1)
        self.assertFalse(np.all(
            op_perp.nematic_tensor == np.diag([-0.5, 1, -0.5])))

    def test_repr(self):
        u = np.array([1, 0, 0])
        op = freud.order.NematicOrderParameter(u)
        self.assertEqual(str(op), str(eval(repr(op))))


if __name__ == '__main__':
    unittest.main()

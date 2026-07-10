import unittest
from datetime import datetime, timedelta

from appointment_system import AppointmentSystem


class AppointmentSystemTests(unittest.TestCase):
    def setUp(self) -> None:
        self.system = AppointmentSystem()
        self.base_time = datetime(2026, 1, 1, 9, 0, 0)

    def test_create_appointment_success(self) -> None:
        appointment = self.system.create_appointment(
            user_name="Alice",
            start_time=self.base_time,
            end_time=self.base_time + timedelta(hours=1),
            description="Dentist",
        )

        self.assertEqual(appointment.id, 1)
        self.assertEqual(appointment.user_name, "Alice")
        self.assertEqual(len(self.system.list_appointments()), 1)

    def test_create_appointment_rejects_overlap(self) -> None:
        self.system.create_appointment(
            user_name="Alice",
            start_time=self.base_time,
            end_time=self.base_time + timedelta(hours=1),
        )

        with self.assertRaises(ValueError):
            self.system.create_appointment(
                user_name="Bob",
                start_time=self.base_time + timedelta(minutes=30),
                end_time=self.base_time + timedelta(hours=2),
            )

    def test_cancel_appointment(self) -> None:
        appointment = self.system.create_appointment(
            user_name="Alice",
            start_time=self.base_time,
            end_time=self.base_time + timedelta(hours=1),
        )

        self.assertTrue(self.system.cancel_appointment(appointment.id))
        self.assertFalse(self.system.cancel_appointment(appointment.id))
        self.assertEqual(self.system.list_appointments(), [])


if __name__ == "__main__":
    unittest.main()

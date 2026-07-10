from __future__ import annotations

from dataclasses import dataclass
from datetime import datetime
from typing import Dict, List


@dataclass(frozen=True)
class Appointment:
    id: int
    user_name: str
    start_time: datetime
    end_time: datetime
    description: str = ""


class AppointmentSystem:
    def __init__(self) -> None:
        self._appointments: Dict[int, Appointment] = {}
        self._next_id = 1

    def create_appointment(
        self,
        user_name: str,
        start_time: datetime,
        end_time: datetime,
        description: str = "",
    ) -> Appointment:
        if not user_name.strip():
            raise ValueError("user_name cannot be empty")
        if end_time <= start_time:
            raise ValueError("end_time must be later than start_time")

        for existing in self._appointments.values():
            if self._is_overlapping(start_time, end_time, existing.start_time, existing.end_time):
                raise ValueError("appointment time overlaps with an existing appointment")

        appointment = Appointment(
            id=self._next_id,
            user_name=user_name,
            start_time=start_time,
            end_time=end_time,
            description=description,
        )
        self._appointments[appointment.id] = appointment
        self._next_id += 1
        return appointment

    def list_appointments(self) -> List[Appointment]:
        return sorted(self._appointments.values(), key=lambda item: item.start_time)

    def cancel_appointment(self, appointment_id: int) -> bool:
        return self._appointments.pop(appointment_id, None) is not None

    @staticmethod
    def _is_overlapping(
        start_a: datetime,
        end_a: datetime,
        start_b: datetime,
        end_b: datetime,
    ) -> bool:
        return start_a < end_b and end_a > start_b

# appointment_systemv1.0

一个简单的预约系统示例，支持：

- 创建预约
- 查看预约列表（按开始时间排序）
- 取消预约
- 防止预约时间冲突

## 快速使用

```python
from datetime import datetime
from appointment_system import AppointmentSystem

system = AppointmentSystem()
appointment = system.create_appointment(
    user_name="Alice",
    start_time=datetime(2026, 1, 1, 9, 0),
    end_time=datetime(2026, 1, 1, 10, 0),
    description="牙科预约",
)

print(appointment)
print(system.list_appointments())
system.cancel_appointment(appointment.id)
```

## 运行测试

```bash
python -m unittest discover -s tests -v
```

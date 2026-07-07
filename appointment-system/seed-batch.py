#!/usr/bin/env python3
import requests
import json
import time

# API base URL
BASE_URL = 'http://localhost:8080/api'

# 服务商数据
providers_data = [
    {"username": "doctor_li", "password": "123456", "email": "li@clinic.com", "phone": "13800138001", 
     "provider_name": "李明内科诊所", "category": "医疗", "description": "专业内科诊疗，经验丰富的医师团队，提供常见病诊疗和健康咨询服务", "address": "北京市朝阳区健康路88号", "provider_phone": "010-88881111"},
    {"username": "beauty_wang", "password": "123456", "email": "wang@beauty.com", "phone": "13800138002",
     "provider_name": "王晓美容SPA", "category": "美容", "description": "高品质美容护理，环境优雅，技师专业，让您焕发美丽", "address": "上海市静安区美容街12号", "provider_phone": "021-88882222"},
    {"username": "fitness_zhang", "password": "123456", "email": "zhang@gym.com", "phone": "13800138003",
     "provider_name": "张教练健身工作室", "category": "健身", "description": "专业健身指导，减脂塑形增肌，一对一私教小班课", "address": "广州市天河区运动公园", "provider_phone": "020-88883333"},
    {"username": "math_zhao", "password": "123456", "email": "zhao@edu.com", "phone": "13800138004",
     "provider_name": "赵老师数学培优", "category": "教育", "description": "一线名师，提分效果显著，个性化辅导方案", "address": "深圳市南山区科技园", "provider_phone": "0755-88884444"},
    {"username": "cleaning_liu", "password": "123456", "email": "liu@clean.com", "phone": "13800138005",
     "provider_name": "刘阿姨家政服务", "category": "家政", "description": "专业家政保洁，多年经验，服务周到细致", "address": "杭州市西湖区", "provider_phone": "0571-88885555"},
    {"username": "lawyer_chen", "password": "123456", "email": "chen@law.com", "phone": "13800138006",
     "provider_name": "陈律师法律咨询", "category": "法律", "description": "资深律师，专业法律咨询，合同审核，纠纷处理", "address": "北京市朝阳区国贸中心", "provider_phone": "010-88886666"},
    {"username": "dental_zhou", "password": "123456", "email": "zhou@dental.com", "phone": "13800138007",
     "provider_name": "周牙医牙科诊所", "category": "医疗", "description": "专业牙科诊疗，洗牙补牙拔牙，环境干净舒适", "address": "成都市锦江区", "provider_phone": "028-88887777"},
    {"username": "hair_sun", "password": "123456", "email": "sun@hair.com", "phone": "13800138008",
     "provider_name": "孙哥美发造型", "category": "美容", "description": "潮流发型设计，烫染护理，资深发型师", "address": "重庆市江北区", "provider_phone": "023-88888888"},
    {"username": "yoga_wu", "password": "123456", "email": "wu@yoga.com", "phone": "13800138009",
     "provider_name": "吴老师瑜伽馆", "category": "健身", "description": "专业瑜伽教学，零基础入门，理疗调理", "address": "苏州市工业园区", "provider_phone": "0512-88889999"},
    {"username": "english_zheng", "password": "123456", "email": "zheng@eng.com", "phone": "13800138010",
     "provider_name": "郑老师英语培训", "category": "教育", "description": "英语口语培训，雅思备考，成人日常英语", "address": "南京市鼓楼区", "provider_phone": "025-88881010"},
    {"username": "baby_qian", "password": "123456", "email": "qian@baby.com", "phone": "13800138011",
     "provider_name": "钱阿姨育儿嫂", "category": "家政", "description": "专业育儿嫂，经验丰富，照顾新生儿和月子护理", "address": "武汉市洪山区", "provider_phone": "027-88881111"},
    {"username": "ip_law", "password": "123456", "email": "guo@ip.com", "phone": "13800138012",
     "provider_name": "郭律师知识产权", "category": "法律", "description": "知识产权专业律师，商标专利咨询侵权分析", "address": "上海市浦东新区陆家嘴", "provider_phone": "021-88881212"},
    {"username": "tcm_huang", "password": "123456", "email": "huang@tcm.com", "phone": "13800138013",
     "provider_name": "黄中医推拿馆", "category": "医疗", "description": "传统中医推拿，经络疏通，颈椎调理", "address": "西安市雁塔区", "provider_phone": "029-88881313"},
    {"username": "nail_ye", "password": "123456", "email": "ye@nail.com", "phone": "13800138014",
     "provider_name": "叶子美甲美睫", "category": "美容", "description": "精致美甲美睫，款式多样，持久耐用", "address": "长沙市岳麓区", "provider_phone": "0731-88881414"},
    {"username": "swim_deng", "password": "123456", "email": "deng@swim.com", "phone": "13800138015",
     "provider_name": "邓教练游泳培训", "category": "健身", "description": "专业游泳教练，成人零基础少儿启蒙，包教包会", "address": "济南市奥体中心", "provider_phone": "0531-88881515"},
    {"username": "it_jiang", "password": "123456", "email": "jiang@it.com", "phone": "13800138016",
     "provider_name": "江老师IT编程", "category": "教育", "description": "实战编程教学，Python前端开发，求职面试辅导", "address": "杭州市余杭区", "provider_phone": "0571-88881616"},
]

# 每个服务商对应的服务
services_data = [
    # doctor_li (医疗)
    [
        {"name": "普通门诊问诊", "price": 50, "duration": 15, "description": "内科常见病初步诊断，开具处方建议"},
        {"name": "健康体检套餐", "price": 299, "duration": 60, "description": "基础血常规、血压、心率全面检查"},
        {"name": "慢性病随访", "price": 80, "duration": 20, "description": "高血压糖尿病慢性病长期随访管理"},
    ],
    # beauty_wang (美容)
    [
        {"name": "全身经络推拿", "price": 168, "duration": 60, "description": "专业技师全身推拿放松，缓解肌肉疲劳"},
        {"name": "面部补水护理", "price": 198, "duration": 45, "description": "深层清洁补水，改善干燥肌肤"},
        {"name": "背部拔罐排毒", "price": 98, "duration": 30, "description": "传统拔罐理疗，祛湿排毒"},
    ],
    # fitness_zhang (健身)
    [
        {"name": "一对一私教课", "price": 260, "duration": 60, "description": "定制健身计划，专业教练一对一指导"},
        {"name": "减脂塑形小班", "price": 120, "duration": 60, "description": "4-6人小班课，针对性减脂塑形"},
        {"name": "拉伸放松", "price": 80, "duration": 30, "description": "运动后深度拉伸，预防肌肉酸痛"},
    ],
    # math_zhao (教育)
    [
        {"name": "初中一对一", "price": 280, "duration": 90, "description": "针对性查漏补缺，提升解题能力"},
        {"name": "中考冲刺班", "price": 180, "duration": 90, "description": "毕业班冲刺训练，真题讲解"},
        {"name": "竞赛辅导", "price": 350, "duration": 120, "description": "奥数竞赛专项提高"},
    ],
    # cleaning_liu (家政)
    [
        {"name": "日常家庭保洁", "price": 120, "duration": 120, "description": "标准三室一厅日常保洁清洁"},
        {"name": "深度开荒保洁", "price": 480, "duration": 300, "description": "新房装修后深度开荒清洁"},
        {"name": "窗户玻璃清洗", "price": 80, "duration": 60, "description": "全屋窗户玻璃专业清洗"},
    ],
    # lawyer_chen (法律)
    [
        {"name": "普通法律咨询", "price": 200, "duration": 30, "description": "一般法律问题在线咨询解答"},
        {"name": "合同审核修改", "price": 800, "duration": 60, "description": "商务合同专业审核修改"},
        {"name": "劳动纠纷代理", "price": 2000, "duration": 0, "description": "劳动仲裁案件全程代理"},
    ],
    # dental_zhou (医疗)
    [
        {"name": "超声波洁牙", "price": 168, "duration": 45, "description": "专业洗牙清洁牙结石"},
        {"name": "补牙修复", "price": 350, "duration": 45, "description": "纳米树脂补牙修复"},
        {"name": "牙齿检查", "price": 50, "duration": 20, "description": "全面口腔检查，拍片分析"},
    ],
    # hair_sun (美容)
    [
        {"name": "剪发造型", "price": 68, "duration": 30, "description": "精剪设计，个性造型"},
        {"name": "烫染套餐", "price": 368, "duration": 150, "description": "潮流烫染，发质护理"},
        {"name": "头发护理", "price": 128, "duration": 40, "description": "营养发膜护理，修复受损"},
    ],
    # yoga_wu (健身)
    [
        {"name": "基础瑜伽小班", "price": 80, "duration": 60, "description": "适合初学者，体位纠正"},
        {"name": "高温瑜伽", "price": 100, "duration": 60, "description": "高温环境促进代谢排毒"},
        {"name": "私教理疗瑜伽", "price": 240, "duration": 60, "description": "一对一私教，针对性调理"},
    ],
    # english_zheng (教育)
    [
        {"name": "少儿英语口语", "price": 180, "duration": 60, "description": "趣味教学，提升听说能力"},
        {"name": "雅思听力阅读", "price": 260, "duration": 90, "description": "雅思考试专项提高"},
        {"name": "成人日常英语", "price": 160, "duration": 60, "description": "职场日常应用英语教学"},
    ],
    # baby_qian (家政)
    [
        {"name": "住家育儿嫂", "price": 8000, "duration": 0, "description": "全天候照顾新生儿，月子护理"},
        {"name": "白天育儿服务", "price": 4000, "duration": 0, "description": "白天8小时婴幼儿照顾陪护"},
    ],
    # ip_law (法律)
    [
        {"name": "商标注册咨询", "price": 500, "duration": 30, "description": "商标注册流程和类别咨询"},
        {"name": "专利侵权分析", "price": 2000, "duration": 60, "description": "侵权风险分析，应对建议"},
    ],
    # tcm_huang (医疗)
    [
        {"name": "中式全身推拿", "price": 128, "duration": 60, "description": "传统中式推拿，疏通经络"},
        {"name": "颈椎牵引调理", "price": 88, "duration": 30, "description": "颈椎问题针对性牵引调理"},
    ],
    # nail_ye (美容)
    [
        {"name": "纯色美甲", "price": 98, "duration": 45, "description": "纯色款式美甲，持久耐用"},
        {"name": "日式嫁接睫毛", "price": 168, "duration": 60, "description": "自然款日式睫毛嫁接"},
    ],
    # swim_deng (健身)
    [
        {"name": "成人一对一", "price": 180, "duration": 60, "description": "成人零基础包会班"},
        {"name": "儿童少儿班", "price": 120, "duration": 60, "description": "儿童安全游泳启蒙"},
    ],
    # it_jiang (教育)
    [
        {"name": "Python入门", "price": 200, "duration": 90, "description": "零基础Python编程入门"},
        {"name": "前端开发项目", "price": 260, "duration": 120, "description": "实战项目教学，求职准备"},
    ],
]


def main():
    print("=== 悦预约批量录入服务商和服务 ===")
    print(f"准备录入 {len(providers_data)} 个服务商...\n")

    success_count = 0
    fail_count = 0

    for i, (provider_info, services_list) in enumerate(zip(providers_data, services_data)):
        print(f"\n[{i+1}/{len(providers_data)}] 正在处理: {provider_info['provider_name']}")

        # 1. 注册用户
        register_data = {
            "username": provider_info["username"],
            "password": provider_info["password"],
            "email": provider_info["email"],
            "phone": provider_info["phone"],
            "role": "provider"
        }

        try:
            r = requests.post(f"{BASE_URL}/auth/register", json=register_data, timeout=10)
            if r.status_code != 200:
                print(f"  × 用户注册失败: {r.text}")
                fail_count += 1
                continue

            result = r.json()
            token = result["token"]
            print(f"  √ 用户注册成功，token: {token[:20]}...")

            # 2. 注册服务商信息
            provider_reg_data = {
                "name": provider_info["provider_name"],
                "category": provider_info["category"],
                "description": provider_info["description"],
                "address": provider_info["address"],
                "phone": provider_info["provider_phone"]
            }

            headers = {"Authorization": f"Bearer {token}"}
            r2 = requests.post(f"{BASE_URL}/providers", json=provider_reg_data, headers=headers, timeout=10)
            if r2.status_code != 200:
                print(f"  × 服务商注册失败: {r2.text}")
                fail_count += 1
                continue

            print(f"  √ 服务商信息注册成功")

            # 3. 添加所有服务
            service_count = 0
            for service in services_list:
                service_data = {
                    "name": service["name"],
                    "category": provider_info["category"],
                    "description": service["description"],
                    "price": service["price"],
                    "duration": service["duration"] if service["duration"] > 0 else 60
                }
                r3 = requests.post(f"{BASE_URL}/services", json=service_data, headers=headers, timeout=10)
                if r3.status_code == 200:
                    service_count += 1
                else:
                    print(f"    - 服务 '{service['name']}' 添加失败: {r3.text}")
                time.sleep(0.2)  # 避免太快

            print(f"  √ 成功添加 {service_count} 个服务")
            success_count += 1

        except Exception as e:
            print(f"  × 请求异常: {str(e)}")
            fail_count += 1

        time.sleep(0.5)

    print(f"\n=== 批量录入完成 ===")
    print(f"成功: {success_count} 个服务商")
    print(f"失败: {fail_count} 个服务商")
    print("\n所有服务商账号密码表格请查看 seed-data.md")

if __name__ == "__main__":
    main()

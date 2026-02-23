-- 创建数据库
CREATE DATABASE IF NOT EXISTS equipment_management;
USE equipment_management;

-- 1. 设备信息表（用于系统管理的已投入设备）
CREATE TABLE IF NOT EXISTS equipments (
    id INT AUTO_INCREMENT PRIMARY KEY,
    equipment_id VARCHAR(50) UNIQUE NOT NULL,
    equipment_name VARCHAR(100) NOT NULL,
    equipment_type VARCHAR(20) NOT NULL,
    location VARCHAR(100) NOT NULL,
    status VARCHAR(20) DEFAULT 'offline',
    power_state VARCHAR(20) DEFAULT 'off',
    energy_total DECIMAL(12,4) DEFAULT 0.0000 COMMENT '累计能耗(0.1kWh)',
    created_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB;

-- 2. 用户表
CREATE TABLE IF NOT EXISTS users (
    id INT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    role VARCHAR(20) NOT NULL,
    real_name VARCHAR(50),
    department VARCHAR(100),
    created_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB;

-- 3. 设备状态记录表
CREATE TABLE IF NOT EXISTS equipment_status_logs (
    id INT AUTO_INCREMENT PRIMARY KEY,
    equipment_id VARCHAR(50) NOT NULL,
    status VARCHAR(20) NOT NULL,
    power_state VARCHAR(20),
    additional_data TEXT,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_equipment_time (equipment_id, timestamp),
    FOREIGN KEY (equipment_id) REFERENCES equipments(equipment_id) ON DELETE CASCADE
) ENGINE=InnoDB;

-- 重建预约表（场所版）
CREATE TABLE IF NOT EXISTS reservations (
    id INT AUTO_INCREMENT PRIMARY KEY,
    place_id VARCHAR(50) NOT NULL COMMENT '场所ID',
    user_id INT NOT NULL,
    purpose VARCHAR(200) NOT NULL,
    start_time DATETIME NOT NULL,
    end_time DATETIME NOT NULL,
    status VARCHAR(20) DEFAULT 'pending' COMMENT 'pending/approved/rejected',
    created_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_place_time (place_id, start_time, end_time),
    INDEX idx_user (user_id),
    FOREIGN KEY (place_id) REFERENCES places(place_id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
) ENGINE=InnoDB;

-- 5. 能耗记录表
CREATE TABLE energy_logs (
    id INT AUTO_INCREMENT PRIMARY KEY,
    equipment_id VARCHAR(50) NOT NULL,
    power_consumption DECIMAL(10,2) NOT NULL,  -- 瞬时功耗(W)
    timestamp DATETIME NOT NULL,               -- 采样时间
    created_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_equipment_timestamp (equipment_id, timestamp),
    FOREIGN KEY (equipment_id) REFERENCES equipments(equipment_id) ON DELETE CASCADE
) ENGINE=InnoDB;

-- 6. 真实设备信息表（简化版，去掉pending概念）
CREATE TABLE IF NOT EXISTS real_equipments (
    id INT AUTO_INCREMENT PRIMARY KEY,
    equipment_id VARCHAR(50) UNIQUE NOT NULL COMMENT '设备唯一标识',
    equipment_name VARCHAR(100) NOT NULL COMMENT '设备名称',
    equipment_type VARCHAR(20) NOT NULL COMMENT '设备类型',
    location VARCHAR(100) NOT NULL COMMENT '安装位置',
    manufacturer VARCHAR(100) COMMENT '制造商',
    model VARCHAR(50) COMMENT '型号',
    serial_number VARCHAR(100) COMMENT '序列号',
    purchase_date DATE COMMENT '采购日期',
    warranty_period INT COMMENT '保修期(月)',
    register_status ENUM('registered', 'unregistered') DEFAULT 'unregistered' COMMENT '注册状态: registered=已投入, unregistered=未投入',
    description TEXT COMMENT '设备描述',
    created_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_register_status (register_status),
    INDEX idx_equipment_type (equipment_type)
) ENGINE=InnoDB;

-- 7. 告警记录表
CREATE TABLE IF NOT EXISTS alarms (
    id INT AUTO_INCREMENT PRIMARY KEY,
    alarm_type VARCHAR(50) NOT NULL COMMENT '告警类型: offline/energy_threshold',
    equipment_id VARCHAR(50) NOT NULL COMMENT '设备ID',
    severity VARCHAR(20) DEFAULT 'warning' COMMENT '级别: warning/error/critical',
    message TEXT NOT NULL,
    is_acknowledged BOOLEAN DEFAULT FALSE,
    created_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_equipment (equipment_id),
    INDEX idx_created_time (created_time)
) ENGINE=InnoDB;

-- 8. 创建场所表
CREATE TABLE IF NOT EXISTS places (
    place_id VARCHAR(50) PRIMARY KEY COMMENT '场所ID',
    place_name VARCHAR(100) NOT NULL COMMENT '场所名称',
    equipment_ids TEXT NOT NULL COMMENT '设备ID列表（逗号分隔如: projector_101,ac_101）',
    location VARCHAR(100) COMMENT '位置描述',
    created_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB;

-- 9. 阈值配置表
CREATE TABLE IF NOT EXISTS thresholds (
    id INT AUTO_INCREMENT PRIMARY KEY,
    equipment_id VARCHAR(50) NOT NULL COMMENT '设备ID',
    threshold_type VARCHAR(50) NOT NULL COMMENT '阈值类型，如 power_threshold',
    threshold_value FLOAT NOT NULL COMMENT '阈值数值',
    created_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    UNIQUE KEY unique_equipment_type (equipment_id, threshold_type),
    INDEX idx_equipment (equipment_id)
) ENGINE=InnoDB;

-- 插入测试数据
-- 3. 插入真实设备数据（设备池）
INSERT INTO real_equipments 
(equipment_id, equipment_name, equipment_type, location, manufacturer, model, register_status) VALUES
('projector_101', '投影仪101', 'projector', '教学楼101', 'Sony', 'VPL-DX120', 'registered'),
('projector_201', '投影仪201', 'projector', '实验室201', 'Epson', 'EB-X49', 'registered'),
('ac_101', '空调101', 'air_conditioner', '教学楼101', 'Gree', 'KFR-35GW', 'registered'),
('ac_201', '空调201', 'air_conditioner', '实验室201', 'Midea', 'KFR-26GW', 'registered'),
('camera_101', '摄像头101', 'camera', '体育馆入口', 'Hikvision', 'DS-2CD2343G0-I', 'registered'),
('door_101', '门禁101', 'access_control', '行政楼大厅', 'Dahua', 'DH-ASI3213Y', 'registered'),

-- 未投入设备（作为扩展示例）
('projector_301', '投影仪301', 'projector', '教学楼301', 'BenQ', 'MS560', 'unregistered'),
('ac_301', '空调301', 'air_conditioner', '教学楼301', 'Haier', 'KFR-50LW', 'unregistered');

-- 4. 插入系统管理设备（仅包含已注册设备，与real_equipments保持一致）
INSERT INTO equipments 
(equipment_id, equipment_name, equipment_type, location, status, power_state, energy_total) VALUES
('projector_101', '投影仪101', 'projector', '教学楼101', 'offline', 'off', 0.0000),
('projector_201', '投影仪201', 'projector', '实验室201', 'offline', 'off', 0.0000),
('ac_101', '空调101', 'air_conditioner', '教学楼101', 'offline', 'off', 0.0000),
('ac_201', '空调201', 'air_conditioner', '实验室201', 'offline', 'off', 0.0000),
('camera_101', '摄像头101', 'camera', '体育馆入口', 'offline', 'off', 0.0000),
('door_101', '门禁101', 'access_control', '行政楼大厅', 'offline', 'off', 0.0000);

-- 5. 插入场所数据（equipment_ids必须存在于equipments表）
INSERT INTO places 
(place_id, place_name, equipment_ids, location) VALUES
('classroom_101', '101教室', 'projector_101,ac_101', '教学楼1楼'),
('classroom_201', '201实验室', 'projector_201,ac_201', '实验楼2楼'),
('gymnasium_001', '体育馆', 'camera_101', '体育馆主馆'),
('office_001', '行政楼大厅', 'door_101', '行政楼1楼');

-- 6. 插入用户账号（密码暂用明文，生产环境应使用哈希）
INSERT INTO users 
(username, password_hash, role, real_name, department) VALUES
('student1', '123456', 'student', '测试学生', '计算机学院'),
('teacher1', '123456', 'teacher', '测试老师', '信息工程学院'),
('admin', 'admin123', 'admin', '系统管理员', '信息技术部');
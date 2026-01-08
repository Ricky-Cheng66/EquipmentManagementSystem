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

-- 4. 预约记录表
CREATE TABLE IF NOT EXISTS reservations (
    id INT AUTO_INCREMENT PRIMARY KEY,
    equipment_id VARCHAR(50) NOT NULL,
    user_id INT NOT NULL,
    purpose VARCHAR(200) NOT NULL,
    start_time DATETIME NOT NULL,
    end_time DATETIME NOT NULL,
    status VARCHAR(20) DEFAULT 'pending',
    created_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_equipment_time (equipment_id, start_time, end_time),
    INDEX idx_user (user_id),
    FOREIGN KEY (equipment_id) REFERENCES equipments(equipment_id) ON DELETE CASCADE,
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

-- 插入测试用户数据
INSERT INTO users (username, password_hash, role, real_name, department) VALUES
('admin', 'hashed_password', 'admin', '系统管理员', '信息技术部'),
('teacher1', 'hashed_password', 'teacher', '张老师', '计算机学院'),
('student1', 'hashed_password', 'student', '李同学', '计算机学院');

-- 插入真实设备数据
INSERT INTO real_equipments (equipment_id, equipment_name, equipment_type, location, manufacturer, model, register_status) VALUES
-- 已投入设备（4个，与equipments表保持一致）
('projector_101', '投影仪101', 'projector', '教学楼101', 'Sony', 'VPL-DX120', 'registered'),
('ac_201', '空调201', 'air_conditioner', '实验室201', 'Gree', 'KFR-35GW', 'registered'),
('door_301', '门禁301', 'access_control', '行政楼301', 'Hikvision', 'DS-K1T341', 'registered'),
('camera_401', '摄像头401', 'camera', '图书馆大厅', 'Dahua', 'IPC-HFW1230S', 'registered'),
-- 未投入设备（2个，作为摆设）
('projector_501', '投影仪501', 'projector', '教学楼501', 'Epson', 'CB-X49', 'unregistered'),
('ac_601', '空调601', 'air_conditioner', '实验室601', 'Midea', 'KFR-26GW', 'unregistered');

-- 插入系统管理设备数据（与real_equipments中的已投入设备对应，状态一致）
INSERT INTO equipments (equipment_id, equipment_name, equipment_type, location, status, power_state) VALUES
('projector_101', '投影仪101', 'projector', '教学楼101', 'offline', 'off'),
('ac_201', '空调201', 'air_conditioner', '实验室201', 'offline', 'off'),
('door_301', '门禁301', 'access_control', '行政楼301', 'offline', 'off'),
('camera_401', '摄像头401', 'camera', '图书馆大厅', 'offline', 'off');

-- 注意：这里的状态都设置为offline和off，因为服务器启动时会初始化所有设备状态
-- 实际运行中，设备上线后状态会被更新
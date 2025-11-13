-- 创建数据库
CREATE DATABASE IF NOT EXISTS equipment_management;
USE equipment_management;

-- 1. 设备信息表
CREATE TABLE IF NOT EXISTS equipments (
    id INT AUTO_INCREMENT PRIMARY KEY,
    equipment_id VARCHAR(50) UNIQUE NOT NULL,
    equipment_name VARCHAR(100) NOT NULL,
    equipment_type VARCHAR(20) NOT NULL,
    location VARCHAR(100) NOT NULL,
    status VARCHAR(20) DEFAULT 'offline',
    power_state VARCHAR(20) DEFAULT 'off',
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
    additional_data JSON,
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
CREATE TABLE IF NOT EXISTS energy_logs (
    id INT AUTO_INCREMENT PRIMARY KEY,
    equipment_id VARCHAR(50) NOT NULL,
    power_consumption DECIMAL(10,2) NOT NULL,
    cost DECIMAL(10,2),
    record_date DATE NOT NULL,
    created_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_equipment_date (equipment_id, record_date),
    FOREIGN KEY (equipment_id) REFERENCES equipments(equipment_id) ON DELETE CASCADE
) ENGINE=InnoDB;

-- 插入测试数据
INSERT INTO users (username, password_hash, role, real_name, department) VALUES
('admin', 'hashed_password', 'admin', '系统管理员', '信息技术部'),
('teacher1', 'hashed_password', 'teacher', '张老师', '计算机学院'),
('student1', 'hashed_password', 'student', '李同学', '计算机学院');

INSERT INTO equipments (equipment_id, equipment_name, equipment_type, location, status, power_state) VALUES
('projector_101', '投影仪101', 'projector', '教学楼101', 'online', 'on'),
('ac_201', '空调201', 'air_conditioner', '实验室201', 'online', 'cool'),
('door_301', '门禁301', 'access_control', '行政楼301', 'offline', 'locked');
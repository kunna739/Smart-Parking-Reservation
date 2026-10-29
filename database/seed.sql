-- ====================================================================
-- Initial Seed Data for Smart Parking Reservation & Management System
-- ====================================================================

-- Insert Initial Facility Configuration
INSERT OR IGNORE INTO facilities (facility_id, name, total_slots, car_rate, bike_rate, ev_rate)
VALUES (1, 'Downtown Smart Parking Facility', 35, 250.0, 100.0, 200.0);

-- Insert 20 Car Slots (C-01 to C-20)
INSERT OR IGNORE INTO slots (slot_id, slot_number, category, status) VALUES
(1, 'C-01', 'CAR', 'AVAILABLE'),
(2, 'C-02', 'CAR', 'AVAILABLE'),
(3, 'C-03', 'CAR', 'AVAILABLE'),
(4, 'C-04', 'CAR', 'AVAILABLE'),
(5, 'C-05', 'CAR', 'AVAILABLE'),
(6, 'C-06', 'CAR', 'AVAILABLE'),
(7, 'C-07', 'CAR', 'AVAILABLE'),
(8, 'C-08', 'CAR', 'AVAILABLE'),
(9, 'C-09', 'CAR', 'AVAILABLE'),
(10, 'C-10', 'CAR', 'AVAILABLE'),
(11, 'C-11', 'CAR', 'AVAILABLE'),
(12, 'C-12', 'CAR', 'AVAILABLE'),
(13, 'C-13', 'CAR', 'AVAILABLE'),
(14, 'C-14', 'CAR', 'AVAILABLE'),
(15, 'C-15', 'CAR', 'AVAILABLE'),
(16, 'C-16', 'CAR', 'AVAILABLE'),
(17, 'C-17', 'CAR', 'AVAILABLE'),
(18, 'C-18', 'CAR', 'AVAILABLE'),
(19, 'C-19', 'CAR', 'AVAILABLE'),
(20, 'C-20', 'CAR', 'AVAILABLE');

-- Insert 10 Bike Slots (B-01 to B-10)
INSERT OR IGNORE INTO slots (slot_id, slot_number, category, status) VALUES
(21, 'B-01', 'BIKE', 'AVAILABLE'),
(22, 'B-02', 'BIKE', 'AVAILABLE'),
(23, 'B-03', 'BIKE', 'AVAILABLE'),
(24, 'B-04', 'BIKE', 'AVAILABLE'),
(25, 'B-05', 'BIKE', 'AVAILABLE'),
(26, 'B-06', 'BIKE', 'AVAILABLE'),
(27, 'B-07', 'BIKE', 'AVAILABLE'),
(28, 'B-08', 'BIKE', 'AVAILABLE'),
(29, 'B-09', 'BIKE', 'AVAILABLE'),
(30, 'B-10', 'BIKE', 'AVAILABLE');

-- Insert 5 EV Slots (E-01 to E-05)
INSERT OR IGNORE INTO slots (slot_id, slot_number, category, status) VALUES
(31, 'E-01', 'EV', 'AVAILABLE'),
(32, 'E-02', 'EV', 'AVAILABLE'),
(33, 'E-03', 'EV', 'AVAILABLE'),
(34, 'E-04', 'EV', 'AVAILABLE'),
(35, 'E-05', 'EV', 'AVAILABLE');

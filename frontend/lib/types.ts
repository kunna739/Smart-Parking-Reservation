export type VehicleCategory = 'CAR' | 'BIKE' | 'EV';
export type SlotStatus = 'AVAILABLE' | 'OCCUPIED' | 'RESERVED' | 'MAINTENANCE';

export interface Facility {
  name: string;
  total_slots: number;
  available_slots: number;
  occupied_slots: number;
  reserved_slots: number;
  occupancy_rate: number;
  rates: {
    car: number;
    bike: number;
    ev: number;
  };
}

export interface Slot {
  slot_id: number;
  slot_number: string;
  category: VehicleCategory;
  status: SlotStatus;
  hourly_rate?: number;
  license_plate?: string;
  customer_name?: string;
  customer_phone?: string;
  booking_id?: string;
  session_id?: number;
  entry_time?: number;
  duration_minutes?: number;
  estimated_fee?: number;
}

export interface Customer {
  customer_id: number;
  name: string;
  phone: string;
  email: string;
}

export interface Vehicle {
  vehicle_id: number;
  customer_id: number;
  license_plate: string;
  category: VehicleCategory;
}

export interface ActiveVehicle {
  license_plate: string;
  booking_id: string;
  slot_id: number;
  slot_number: string;
  category: VehicleCategory;
  entry_time: number;
}

export interface WaitingCustomer {
  position: number;
  customer_id: number;
  customer_name: string;
  phone: string;
  license_plate: string;
  category: VehicleCategory;
}

export interface HistoryEvent {
  event_type: string;
  description: string;
  license_plate: string;
  slot_number: string;
  timestamp: number;
}

export interface ApiResponse<T> {
  success: boolean;
  data?: T;
  error?: string;
  code?: number;
}

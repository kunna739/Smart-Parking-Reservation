import {
  Facility,
  Slot,
  Customer,
  Vehicle,
  ActiveVehicle,
  WaitingCustomer,
  HistoryEvent,
  ApiResponse,
  VehicleCategory,
  SlotStatus,
} from './types';

const API_BASE = typeof window !== 'undefined' ? '' : 'http://127.0.0.1:8080';

async function fetchApi<T>(path: string, options?: RequestInit): Promise<T> {
  const url = `${API_BASE}${path}`;
  const res = await fetch(url, {
    ...options,
    headers: {
      'Content-Type': 'application/json',
      ...options?.headers,
    },
    cache: 'no-store',
  });

  const json: ApiResponse<T> = await res.json();
  if (!json.success) {
    throw new Error(json.error || `Request failed with status ${res.status}`);
  }

  return json.data as T;
}

export const api = {
  // Facility & Slots
  getFacility: () => fetchApi<Facility>('/api/facility'),
  configFacility: (data: {
    name: string;
    car_slots: number;
    bike_slots: number;
    ev_slots: number;
    car_rate: number;
    bike_rate: number;
    ev_rate: number;
  }) => fetchApi<{ message: string }>('/api/facility/config', {
    method: 'POST',
    body: JSON.stringify(data),
  }),
  getSlots: (category?: VehicleCategory, status?: SlotStatus) => {
    const params = new URLSearchParams();
    if (category) params.set('category', category);
    if (status) params.set('status', status);
    const qs = params.toString() ? `?${params.toString()}` : '';
    return fetchApi<Slot[]>(`/api/slots${qs}`);
  },
  getSlotDetail: (slotIdOrNumber: number | string) => {
    const param = typeof slotIdOrNumber === 'number'
      ? `slot_id=${slotIdOrNumber}`
      : `slot_number=${encodeURIComponent(slotIdOrNumber)}`;
    return fetchApi<Slot>(`/api/slots/detail?${param}`);
  },

  // Customers & Vehicles
  getCustomers: () => fetchApi<Customer[]>('/api/customers'),
  registerCustomer: (data: { name: string; phone: string; email: string }) =>
    fetchApi<Customer>('/api/customers', {
      method: 'POST',
      body: JSON.stringify(data),
    }),
  registerVehicle: (data: {
    customer_id: number;
    license_plate: string;
    category: VehicleCategory;
  }) =>
    fetchApi<Vehicle>('/api/vehicles', {
      method: 'POST',
      body: JSON.stringify(data),
    }),
  searchVehicle: (plate: string) =>
    fetchApi<{
      license_plate: string;
      customer_name: string;
      category: VehicleCategory;
      is_parked: boolean;
      slot_id: number;
      slot_number: string;
    }>(`/api/vehicles/search?plate=${encodeURIComponent(plate)}`),

  // Reservations
  createReservation: (data: {
    customer_id: number;
    vehicle_id: number;
    slot_id: number;
  }) =>
    fetchApi<{ booking_id: string; slot_id: number; status: string }>(
      '/api/reservations',
      {
        method: 'POST',
        body: JSON.stringify(data),
      }
    ),
  cancelReservation: (booking_id: string) =>
    fetchApi<{ message: string }>('/api/reservations/cancel', {
      method: 'POST',
      body: JSON.stringify({ booking_id }),
    }),

  // Queue
  getQueue: () => fetchApi<WaitingCustomer[]>('/api/queue'),
  joinQueue: (data: {
    customer_id: number;
    license_plate: string;
    category: VehicleCategory;
  }) =>
    fetchApi<{ queue_position: number; license_plate: string; category: string }>(
      '/api/queue/join',
      {
        method: 'POST',
        body: JSON.stringify(data),
      }
    ),
  admitQueueCustomer: (license_plate: string) =>
    fetchApi<{ slot_number: string; slot_id: number; license_plate: string }>(
      '/api/queue/admit',
      {
        method: 'POST',
        body: JSON.stringify({ license_plate }),
      }
    ),
  leaveQueue: (license_plate: string, customer_id?: number) =>
    fetchApi<{ message: string }>('/api/queue/leave', {
      method: 'POST',
      body: JSON.stringify({ license_plate, customer_id }),
    }),

  // Gate Terminal (Entry / Exit)
  vehicleEntry: (data: {
    booking_id?: string;
    license_plate: string;
    category: VehicleCategory;
    phone?: string;
    customer_name?: string;
  }) =>
    fetchApi<{
      session_id: number;
      slot_number: string;
      slot_id: number;
      license_plate: string;
    }>('/api/parking/entry', {
      method: 'POST',
      body: JSON.stringify(data),
    }),
  vehicleExit: (license_plate: string) =>
    fetchApi<{
      session_id: number;
      license_plate: string;
      slot_number: string;
      duration_minutes: number;
      amount_due: number;
      queue_allocated?: { customer_name: string; license_plate: string };
    }>('/api/parking/exit', {
      method: 'POST',
      body: JSON.stringify({ license_plate }),
    }),
  getActiveVehicles: () => fetchApi<ActiveVehicle[]>('/api/parking/active'),

  // Payments
  processPayment: (data: {
    session_id?: number;
    booking_id?: string;
    license_plate?: string;
    amount: number;
    payment_method: string;
  }) =>
    fetchApi<{
      payment_id: number;
      license_plate?: string;
      amount: number;
      status: string;
      method: string;
    }>('/api/payments', {
      method: 'POST',
      body: JSON.stringify(data),
    }),

  // History & Reports
  getHistory: () => fetchApi<HistoryEvent[]>('/api/history'),
  getSummary: () =>
    fetchApi<{
      total_capacity: number;
      available_slots: number;
      occupied_slots: number;
      reserved_slots: number;
      queue_count: number;
      occupancy_rate: number;
      total_revenue: number;
    }>('/api/reports/summary'),
};

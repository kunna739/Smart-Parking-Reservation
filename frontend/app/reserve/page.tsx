'use client';

import React, { useEffect, useState } from 'react';
import { api } from '@/lib/api';
import { Slot, Customer, VehicleCategory } from '@/lib/types';
import SlotGrid from '@/components/SlotGrid';
import { CalendarCheck, CheckCircle2, Ticket, Car, AlertTriangle } from 'lucide-react';

export default function ReservePage() {
  const [slots, setSlots] = useState<Slot[]>([]);
  const [customers, setCustomers] = useState<Customer[]>([]);
  const [selectedSlot, setSelectedSlot] = useState<Slot | null>(null);
  const [selectedCustomerId, setSelectedCustomerId] = useState<number>(0);
  const [bookingSuccess, setBookingSuccess] = useState<any>(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const loadData = async () => {
    try {
      const [slotsData, custList] = await Promise.all([
        api.getSlots(),
        api.getCustomers(),
      ]);
      setSlots(slotsData);
      setCustomers(custList);
      if (custList.length > 0 && selectedCustomerId === 0) {
        setSelectedCustomerId(custList[0].customer_id);
      }
    } catch (err: any) {
      setError(err.message);
    }
  };

  useEffect(() => {
    loadData();
  }, []);

  const handleBooking = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!selectedSlot) {
      setError('Please select an available parking slot from the layout.');
      return;
    }
    if (selectedSlot.status !== 'AVAILABLE') {
      setError('Selected slot is not available. Please choose another slot.');
      return;
    }

    try {
      setLoading(true);
      setError(null);
      const res = await api.createReservation({
        customer_id: selectedCustomerId,
        vehicle_id: 1, // associated vehicle or auto-referenced
        slot_id: selectedSlot.slot_id,
      });
      setBookingSuccess({
        ...res,
        slot_number: selectedSlot.slot_number,
        category: selectedSlot.category,
      });
      setSelectedSlot(null);
      loadData();
    } catch (err: any) {
      setError(err.message || 'Failed to complete reservation');
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="space-y-8">
      <div>
        <h1 className="text-2xl font-bold text-white tracking-tight flex items-center gap-2">
          <CalendarCheck className="w-6 h-6 text-indigo-400" />
          Reserve a Parking Space
        </h1>
        <p className="text-sm text-slate-400 mt-1">
          Select an available slot from the live visual layout and confirm your reservation.
        </p>
      </div>

      {error && (
        <div className="p-4 bg-rose-950/40 border border-rose-800 rounded-xl text-rose-300 text-sm flex items-center gap-2">
          <AlertTriangle className="w-4 h-4 shrink-0" />
          {error}
        </div>
      )}

      {/* Confirmation Booking Card */}
      {bookingSuccess && (
        <div className="p-6 bg-emerald-950/40 border border-emerald-600/80 rounded-2xl shadow-xl space-y-4">
          <div className="flex items-center gap-2 text-emerald-400 font-semibold text-lg">
            <CheckCircle2 className="w-5 h-5" />
            Reservation Confirmed!
          </div>
          <div className="grid grid-cols-1 sm:grid-cols-3 gap-4 text-sm">
            <div className="p-3 bg-slate-900/60 rounded-xl">
              <span className="text-slate-400 text-xs block">Booking ID</span>
              <span className="font-mono font-bold text-white text-base">
                {bookingSuccess.booking_id}
              </span>
            </div>
            <div className="p-3 bg-slate-900/60 rounded-xl">
              <span className="text-slate-400 text-xs block">Assigned Slot</span>
              <span className="font-mono font-bold text-indigo-400 text-base">
                {bookingSuccess.slot_number} ({bookingSuccess.category})
              </span>
            </div>
            <div className="p-3 bg-slate-900/60 rounded-xl">
              <span className="text-slate-400 text-xs block">Status</span>
              <span className="font-semibold text-emerald-400">
                {bookingSuccess.status}
              </span>
            </div>
          </div>
          <button
            onClick={() => setBookingSuccess(null)}
            className="text-xs text-slate-300 hover:text-white underline"
          >
            Dismiss
          </button>
        </div>
      )}

      {/* Reservation Form & Slot Picker */}
      <div className="grid grid-cols-1 lg:grid-cols-3 gap-8">
        <div className="lg:col-span-2 space-y-4">
          <h2 className="text-base font-semibold text-white">
            1. Select Slot from Live Grid
          </h2>
          <SlotGrid
            slots={slots}
            onSelectSlot={(s) => setSelectedSlot(s)}
            selectedSlotId={selectedSlot?.slot_id}
            disableOccupied={true}
          />
        </div>

        <div className="space-y-4">
          <h2 className="text-base font-semibold text-white">
            2. Customer & Confirmation
          </h2>
          <form
            onSubmit={handleBooking}
            className="p-6 bg-slate-900 border border-slate-800 rounded-2xl space-y-5"
          >
            <div>
              <label className="block text-xs font-medium text-slate-400 mb-1.5">
                Select Registered Customer
              </label>
              {customers.length > 0 ? (
                <select
                  value={selectedCustomerId}
                  onChange={(e) => setSelectedCustomerId(Number(e.target.value))}
                  className="w-full bg-slate-800 border border-slate-700 rounded-lg px-3 py-2 text-sm text-slate-100 focus:outline-none focus:border-indigo-500"
                >
                  {customers.map((c) => (
                    <option key={c.customer_id} value={c.customer_id}>
                      {c.name} ({c.phone})
                    </option>
                  ))}
                </select>
              ) : (
                <div className="text-xs text-amber-400">
                  No customers found. Register one in &apos;Customers &amp; Cars&apos; first.
                </div>
              )}
            </div>

            <div className="p-4 bg-slate-800/60 rounded-xl space-y-2">
              <div className="text-xs text-slate-400">Selected Slot:</div>
              {selectedSlot ? (
                <div className="flex items-center justify-between">
                  <span className="font-mono font-bold text-lg text-white">
                    {selectedSlot.slot_number}
                  </span>
                  <span className="text-xs font-semibold px-2 py-0.5 rounded bg-indigo-900 text-indigo-300">
                    {selectedSlot.category}
                  </span>
                </div>
              ) : (
                <div className="text-xs text-slate-500 italic">
                  Click on an available slot in the layout grid
                </div>
              )}
            </div>

            <button
              type="submit"
              disabled={loading || !selectedSlot || selectedSlot.status !== 'AVAILABLE'}
              className="w-full py-2.5 bg-indigo-600 hover:bg-indigo-500 disabled:bg-slate-800 disabled:text-slate-500 text-white rounded-xl text-sm font-semibold shadow-lg shadow-indigo-600/30 transition flex items-center justify-center gap-2"
            >
              <Ticket className="w-4 h-4" />
              {loading ? 'Confirming...' : 'Confirm Reservation'}
            </button>
          </form>
        </div>
      </div>
    </div>
  );
}

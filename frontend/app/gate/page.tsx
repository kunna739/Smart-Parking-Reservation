'use client';

import React, { useEffect, useState } from 'react';
import { useRouter } from 'next/navigation';
import { api } from '@/lib/api';
import { ActiveVehicle, VehicleCategory, Slot } from '@/lib/types';
import {
  DoorOpen,
  LogIn,
  LogOut,
  Car,
  Bike,
  Zap,
  CheckCircle2,
  Clock,
  CreditCard,
  User,
  Phone,
  Search,
  ArrowRight,
  RefreshCw,
} from 'lucide-react';

export default function GateTerminalPage() {
  const router = useRouter();

  const [activeVehicles, setActiveVehicles] = useState<ActiveVehicle[]>([]);
  const [parkedSlots, setParkedSlots] = useState<Slot[]>([]);

  // Check-In form state
  const [checkinPlate, setCheckinPlate] = useState('');
  const [checkinBookingId, setCheckinBookingId] = useState('');
  const [checkinPhone, setCheckinPhone] = useState('');
  const [checkinCustomerName, setCheckinCustomerName] = useState('');
  const [checkinCategory, setCheckinCategory] = useState<VehicleCategory>('CAR');

  // Check-Out form state
  const [checkoutPlate, setCheckoutPlate] = useState('');
  const [checkoutResult, setCheckoutResult] = useState<any>(null);

  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [successMsg, setSuccessMsg] = useState<string | null>(null);

  const loadData = async () => {
    try {
      const [activeData, slotsData] = await Promise.all([
        api.getActiveVehicles(),
        api.getSlots(),
      ]);
      setActiveVehicles(activeData);
      const occupied = slotsData.filter((s) => s.status === 'OCCUPIED' && s.license_plate);
      setParkedSlots(occupied);
    } catch (err: any) {
      console.error(err);
    }
  };

  useEffect(() => {
    loadData();
    const interval = setInterval(loadData, 5000);
    return () => clearInterval(interval);
  }, []);

  const handleCheckIn = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!checkinPlate.trim() || !checkinPhone.trim() || !checkinCustomerName.trim()) {
      setError('Driver Name, Phone Number, and License Plate are compulsory.');
      return;
    }

    try {
      setLoading(true);
      setError(null);
      setSuccessMsg(null);
      const res = await api.vehicleEntry({
        license_plate: checkinPlate.trim(),
        booking_id: checkinBookingId.trim() || undefined,
        category: checkinCategory,
        phone: checkinPhone.trim(),
        customer_name: checkinCustomerName.trim(),
      });

      setSuccessMsg(
        `Entry Gate Opened! Vehicle ${res.license_plate} assigned to Slot ${res.slot_number}.`
      );
      setCheckinPlate('');
      setCheckinBookingId('');
      setCheckinPhone('');
      setCheckinCustomerName('');
      loadData();
    } catch (err: any) {
      setError(err.message || 'Check-in failed');
    } finally {
      setLoading(false);
    }
  };

  const handleCheckOut = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!checkoutPlate.trim()) {
      setError('Please select a vehicle to check out.');
      return;
    }

    try {
      setLoading(true);
      setError(null);
      setSuccessMsg(null);
      setCheckoutResult(null);

      const res = await api.vehicleExit(checkoutPlate.trim());
      setCheckoutResult(res);
      setCheckoutPlate('');
      loadData();
    } catch (err: any) {
      setError(err.message || 'Check-out failed');
    } finally {
      setLoading(false);
    }
  };

  // Find info of the currently selected exiting vehicle
  const selectedVehicleInfo = parkedSlots.find(
    (s) => s.license_plate?.toUpperCase() === checkoutPlate.trim().toUpperCase()
  );

  return (
    <div className="space-y-8">
      <div>
        <h1 className="text-2xl font-bold text-white tracking-tight flex items-center gap-2">
          <DoorOpen className="w-6 h-6 text-indigo-400" />
          Operator Gate Terminal
        </h1>
        <p className="text-sm text-slate-400 mt-1">
          Manage entry gate barrier check-in and exit gate billing checkout.
        </p>
      </div>

      {error && (
        <div className="p-4 bg-rose-950/40 border border-rose-800 rounded-xl text-rose-300 text-sm flex items-center gap-2">
          <span>⚠️</span>
          <span>{error}</span>
        </div>
      )}

      {successMsg && (
        <div className="p-4 bg-emerald-950/40 border border-emerald-600/80 rounded-xl text-emerald-300 text-sm flex items-center gap-2">
          <CheckCircle2 className="w-4 h-4 shrink-0" />
          <span>{successMsg}</span>
        </div>
      )}

      {/* Exit Billing Summary Alert with Direct Payment Redirect */}
      {checkoutResult && (
        <div className="p-6 bg-gradient-to-br from-indigo-950/60 to-slate-900 border-2 border-indigo-500/80 rounded-2xl space-y-4 shadow-2xl animate-in fade-in zoom-in duration-200">
          <div className="flex items-center justify-between border-b border-indigo-500/30 pb-3">
            <div className="flex items-center gap-2 text-indigo-400 font-bold text-lg">
              <CheckCircle2 className="w-6 h-6 text-emerald-400" />
              Check-Out Completed · Ready for Payment
            </div>
            <span className="text-xs font-mono px-3 py-1 bg-emerald-500/20 text-emerald-300 border border-emerald-500/40 rounded-full font-bold">
              SLOT VACATED
            </span>
          </div>

          <div className="grid grid-cols-2 sm:grid-cols-4 gap-4 text-sm">
            <div className="p-3 bg-slate-900/80 rounded-xl border border-slate-800">
              <span className="text-slate-400 text-xs block">Vehicle Plate</span>
              <span className="font-mono font-bold text-white text-base">
                {checkoutResult.license_plate}
              </span>
            </div>
            <div className="p-3 bg-slate-900/80 rounded-xl border border-slate-800">
              <span className="text-slate-400 text-xs block">Vacated Slot</span>
              <span className="font-mono font-bold text-indigo-300 text-base">
                {checkoutResult.slot_number}
              </span>
            </div>
            <div className="p-3 bg-slate-900/80 rounded-xl border border-slate-800">
              <span className="text-slate-400 text-xs block">Duration</span>
              <span className="font-bold text-white text-base">
                {checkoutResult.duration_minutes} min
              </span>
            </div>
            <div className="p-3 bg-slate-900/80 rounded-xl border border-slate-800">
              <span className="text-slate-400 text-xs block">Amount Due</span>
              <span className="font-black text-emerald-400 text-xl">
                ₹{checkoutResult.amount_due?.toFixed(2)}
              </span>
            </div>
          </div>

          {checkoutResult.queue_allocated && (
            <div className="p-3 bg-amber-950/50 border border-amber-800 rounded-xl text-xs text-amber-300 flex items-center gap-2">
              <Clock className="w-4 h-4 shrink-0" />
              <span>
                <strong>FIFO Queue Auto-Allocation:</strong> Vacated slot has been immediately
                awarded to waiting customer <strong>{checkoutResult.queue_allocated.customer_name}</strong> ({checkoutResult.queue_allocated.license_plate})!
              </span>
            </div>
          )}

          <div className="flex flex-col sm:flex-row sm:items-center sm:justify-between gap-4 pt-3 border-t border-indigo-500/20">
            <button
              onClick={() => setCheckoutResult(null)}
              className="text-xs text-slate-400 hover:text-white underline cursor-pointer"
            >
              Close Summary
            </button>
            <button
              onClick={() =>
                router.push(
                  `/billing?plate=${encodeURIComponent(checkoutResult.license_plate)}&amount=${checkoutResult.amount_due}`
                )
              }
              className="px-6 py-3 bg-emerald-600 hover:bg-emerald-500 active:scale-[0.99] text-white rounded-xl text-sm font-bold transition shadow-xl shadow-emerald-600/30 flex items-center justify-center gap-2 cursor-pointer"
            >
              <CreditCard className="w-4 h-4" />
              <span>Proceed to Payment (Collect ₹{checkoutResult.amount_due?.toFixed(2)})</span>
              <ArrowRight className="w-4 h-4" />
            </button>
          </div>
        </div>
      )}

      {/* Two Terminal Panes */}
      <div className="grid grid-cols-1 md:grid-cols-2 gap-8">
        {/* Check-In Pane */}
        <form
          onSubmit={handleCheckIn}
          className="p-6 bg-slate-900 border border-slate-800 rounded-2xl space-y-4"
        >
          <div className="flex items-center gap-2 text-white font-semibold text-base border-b border-slate-800 pb-3">
            <LogIn className="w-5 h-5 text-emerald-400" />
            Entry Gate (Check-In)
          </div>

          <div>
            <label className="block text-xs font-medium text-slate-400 mb-1">
              Vehicle License Plate *
            </label>
            <input
              type="text"
              required
              placeholder="e.g. DL-01-AB-1234"
              value={checkinPlate}
              onChange={(e) => setCheckinPlate(e.target.value)}
              className="w-full bg-slate-800 border border-slate-700 rounded-lg px-3 py-2 text-sm text-slate-100 uppercase font-mono tracking-wider focus:outline-none focus:border-indigo-500"
            />
          </div>

          <div>
            <label className="block text-xs font-medium text-slate-400 mb-1">
              Driver / Customer Name *
            </label>
            <input
              type="text"
              required
              placeholder="e.g. Rahul Sharma"
              value={checkinCustomerName}
              onChange={(e) => setCheckinCustomerName(e.target.value)}
              className="w-full bg-slate-800 border border-slate-700 rounded-lg px-3 py-2 text-sm text-slate-100 focus:outline-none focus:border-indigo-500"
            />
          </div>

          <div>
            <label className="block text-xs font-medium text-slate-400 mb-1">
              Phone Number *
            </label>
            <input
              type="tel"
              required
              placeholder="e.g. +91 98765 43210"
              value={checkinPhone}
              onChange={(e) => setCheckinPhone(e.target.value)}
              className="w-full bg-slate-800 border border-slate-700 rounded-lg px-3 py-2 text-sm text-slate-100 font-mono focus:outline-none focus:border-indigo-500"
            />
          </div>

          <div>
            <label className="block text-xs font-medium text-slate-400 mb-1">
              Booking ID (Optional - if pre-reserved)
            </label>
            <input
              type="text"
              placeholder="e.g. BK-9482F1"
              value={checkinBookingId}
              onChange={(e) => setCheckinBookingId(e.target.value)}
              className="w-full bg-slate-800 border border-slate-700 rounded-lg px-3 py-2 text-sm text-slate-100 font-mono focus:outline-none focus:border-indigo-500"
            />
          </div>

          <div>
            <label className="block text-xs font-medium text-slate-400 mb-1">
              Vehicle Category
            </label>
            <select
              value={checkinCategory}
              onChange={(e) => setCheckinCategory(e.target.value as VehicleCategory)}
              className="w-full bg-slate-800 border border-slate-700 rounded-lg px-3 py-2 text-sm text-slate-100 focus:outline-none focus:border-indigo-500"
            >
              <option value="CAR">Car (₹250/hr)</option>
              <option value="BIKE">Motorcycle / Bike (₹100/hr)</option>
              <option value="EV">Electric Vehicle - EV (₹200/hr)</option>
            </select>
          </div>

          <button
            type="submit"
            disabled={loading}
            className="w-full py-2.5 bg-emerald-600 hover:bg-emerald-500 text-white rounded-xl text-sm font-semibold shadow-lg shadow-emerald-600/30 transition flex items-center justify-center gap-2 cursor-pointer"
          >
            <LogIn className="w-4 h-4" />
            {loading ? 'Processing Gate...' : 'Open Gate & Check In'}
          </button>
        </form>

        {/* Check-Out Pane with Easy Vehicle & Customer Selector */}
        <form
          onSubmit={handleCheckOut}
          className="p-6 bg-slate-900 border border-slate-800 rounded-2xl space-y-4"
        >
          <div className="flex items-center gap-2 text-white font-semibold text-base border-b border-slate-800 pb-3">
            <LogOut className="w-5 h-5 text-rose-400" />
            Exit Gate (Check-Out)
          </div>

          <div>
            <label className="block text-xs font-bold uppercase tracking-wider text-slate-300 mb-1.5">
              Select Parked Vehicle Exiting *
            </label>

            {activeVehicles.length > 0 ? (
              <select
                value={checkoutPlate}
                onChange={(e) => setCheckoutPlate(e.target.value)}
                className="w-full bg-slate-800 border-2 border-slate-700 focus:border-rose-500 rounded-xl px-3 py-3 text-sm text-slate-100 font-semibold focus:outline-none transition"
              >
                <option value="">-- Choose Parked Vehicle with Customer Name --</option>
                {activeVehicles.map((v) => {
                  const slotInfo = parkedSlots.find(
                    (s) => s.license_plate?.toUpperCase() === v.license_plate.toUpperCase()
                  );
                  const driver = slotInfo?.customer_name || 'Customer';
                  return (
                    <option key={v.license_plate} value={v.license_plate}>
                      {v.license_plate} • Slot {v.slot_number} — {driver} ({v.category})
                    </option>
                  );
                })}
              </select>
            ) : (
              <div className="p-3 bg-slate-800/40 border border-slate-700/60 rounded-xl text-xs text-slate-400">
                No active vehicles currently parked inside.
              </div>
            )}
          </div>

          {/* Or manual plate input for quick search */}
          <div>
            <label className="block text-[11px] text-slate-400 mb-1">
              Or search / enter License Plate manually:
            </label>
            <div className="relative">
              <Search className="w-4 h-4 text-slate-400 absolute left-3 top-2.5" />
              <input
                type="text"
                placeholder="e.g. DL-01-AB-1234"
                value={checkoutPlate}
                onChange={(e) => setCheckoutPlate(e.target.value)}
                className="w-full bg-slate-800 border border-slate-700 rounded-lg pl-9 pr-3 py-2 text-sm text-slate-100 uppercase font-mono tracking-wider focus:outline-none focus:border-rose-500"
              />
            </div>
          </div>

          {/* Selected Vehicle Preview Banner */}
          {selectedVehicleInfo && (
            <div className="p-3.5 bg-rose-950/30 border border-rose-800/60 rounded-xl text-xs space-y-1.5 animate-in fade-in duration-150">
              <div className="flex items-center justify-between">
                <span className="font-mono font-bold text-white text-sm">
                  {selectedVehicleInfo.license_plate}
                </span>
                <span className="px-2 py-0.5 rounded bg-rose-900/60 text-rose-300 font-bold">
                  Slot {selectedVehicleInfo.slot_number}
                </span>
              </div>
              <div className="flex items-center gap-3 text-slate-300">
                <span className="flex items-center gap-1">
                  <User className="w-3 h-3 text-rose-400" />
                  {selectedVehicleInfo.customer_name || 'Customer'}
                </span>
                {selectedVehicleInfo.customer_phone && (
                  <span className="flex items-center gap-1 font-mono text-slate-400">
                    <Phone className="w-3 h-3 text-rose-400" />
                    {selectedVehicleInfo.customer_phone}
                  </span>
                )}
                <span className="text-amber-300 font-semibold">
                  Rate: ₹{selectedVehicleInfo.hourly_rate ?? 250}/hr
                </span>
              </div>
            </div>
          )}

          <div className="p-3 bg-slate-800/40 rounded-xl text-[11px] text-slate-400">
            Checking out will release the slot back to the facility or auto-allocate it to the next queued customer.
          </div>

          <button
            type="submit"
            disabled={loading || !checkoutPlate.trim()}
            className="w-full py-3 bg-rose-600 hover:bg-rose-500 disabled:opacity-50 text-white rounded-xl text-sm font-bold shadow-lg shadow-rose-600/30 transition flex items-center justify-center gap-2 cursor-pointer"
          >
            <LogOut className="w-4 h-4" />
            {loading ? 'Calculating Fee...' : 'Compute Fee & Exit'}
          </button>
        </form>
      </div>

      {/* Currently Parked Active Vehicles Table */}
      <div className="space-y-4">
        <div className="flex items-center justify-between">
          <h2 className="text-base font-semibold text-white">
            Currently Parked Vehicles ({activeVehicles.length})
          </h2>
          <span className="text-xs text-slate-400 font-mono">
            Maintained via C11 Doubly Linked List
          </span>
        </div>

        <div className="overflow-x-auto bg-slate-900 border border-slate-800 rounded-2xl">
          <table className="w-full text-left text-xs">
            <thead className="bg-slate-800/60 text-slate-400 uppercase tracking-wider">
              <tr>
                <th className="py-3 px-4">Slot</th>
                <th className="py-3 px-4">License Plate</th>
                <th className="py-3 px-4">Driver / Customer</th>
                <th className="py-3 px-4">Category</th>
                <th className="py-3 px-4">Booking ID</th>
                <th className="py-3 px-4 text-right">Action</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-slate-800">
              {activeVehicles.map((v) => {
                const slotInfo = parkedSlots.find(
                  (s) => s.license_plate?.toUpperCase() === v.license_plate.toUpperCase()
                );
                const driver = slotInfo?.customer_name || 'Customer';
                return (
                  <tr key={v.license_plate} className="hover:bg-slate-800/40 transition">
                    <td className="py-3 px-4 font-mono font-bold text-indigo-400">
                      {v.slot_number}
                    </td>
                    <td className="py-3 px-4 font-mono font-semibold text-white">
                      {v.license_plate}
                    </td>
                    <td className="py-3 px-4 font-medium text-slate-200">
                      {driver}
                    </td>
                    <td className="py-3 px-4">
                      <span className="px-2 py-0.5 rounded bg-slate-800 text-slate-300">
                        {v.category}
                      </span>
                    </td>
                    <td className="py-3 px-4 font-mono text-slate-400">
                      {v.booking_id || 'Walk-in'}
                    </td>
                    <td className="py-3 px-4 text-right">
                      <button
                        onClick={() => {
                          setCheckoutPlate(v.license_plate);
                          window.scrollTo({ top: 150, behavior: 'smooth' });
                        }}
                        className="px-2.5 py-1 bg-rose-950/60 hover:bg-rose-900 border border-rose-800/80 text-rose-300 rounded-lg text-xs font-semibold transition"
                      >
                        Select for Exit
                      </button>
                    </td>
                  </tr>
                );
              })}
              {activeVehicles.length === 0 && (
                <tr>
                  <td colSpan={6} className="py-8 text-center text-slate-500">
                    No active vehicles parked inside currently.
                  </td>
                </tr>
              )}
            </tbody>
          </table>
        </div>
      </div>
    </div>
  );
}

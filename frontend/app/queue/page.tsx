'use client';

import React, { useEffect, useState } from 'react';
import { api } from '@/lib/api';
import { WaitingCustomer, Customer, VehicleCategory } from '@/lib/types';
import {
  Clock,
  PlusCircle,
  CheckCircle2,
  AlertCircle,
  Trash2,
  LogIn,
  Car,
  Info,
} from 'lucide-react';

export default function QueuePage() {
  const [queue, setQueue] = useState<WaitingCustomer[]>([]);
  const [customers, setCustomers] = useState<Customer[]>([]);
  const [selectedCustomerId, setSelectedCustomerId] = useState<number>(0);
  const [queuePlate, setQueuePlate] = useState('');
  const [queueCategory, setQueueCategory] = useState<VehicleCategory>('CAR');
  const [availableSlots, setAvailableSlots] = useState<number>(0);

  const [loading, setLoading] = useState(false);
  const [actionLoading, setActionLoading] = useState<string | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [successMsg, setSuccessMsg] = useState<string | null>(null);

  const loadData = async () => {
    try {
      const [qData, custList, slotsData] = await Promise.all([
        api.getQueue(),
        api.getCustomers(),
        api.getSlots(),
      ]);
      setQueue(qData);
      setCustomers(custList);
      const freeCount = slotsData.filter((s) => s.status === 'AVAILABLE').length;
      setAvailableSlots(freeCount);

      if (custList.length > 0 && selectedCustomerId === 0) {
        setSelectedCustomerId(custList[0].customer_id);
      }
    } catch (err: any) {
      console.error(err);
    }
  };

  useEffect(() => {
    loadData();
    const interval = setInterval(loadData, 5000);
    return () => clearInterval(interval);
  }, []);

  const handleJoinQueue = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!queuePlate.trim() || selectedCustomerId <= 0) return;

    try {
      setLoading(true);
      setError(null);
      setSuccessMsg(null);
      const res = await api.joinQueue({
        customer_id: selectedCustomerId,
        license_plate: queuePlate.trim(),
        category: queueCategory,
      });

      setSuccessMsg(
        `Added to waiting queue at position #${res.queue_position} for ${res.category} slot.`
      );
      setQueuePlate('');
      loadData();
    } catch (err: any) {
      setError(err.message || 'Failed to join queue');
    } finally {
      setLoading(false);
    }
  };

  const handleAdmit = async (plate: string) => {
    try {
      setActionLoading(plate);
      setError(null);
      setSuccessMsg(null);

      const res = await api.admitQueueCustomer(plate);
      setSuccessMsg(
        `Vehicle ${plate} has been admitted directly into Slot ${res.slot_number}!`
      );
      loadData();
    } catch (err: any) {
      setError(err.message || 'Failed to admit vehicle to slot');
    } finally {
      setActionLoading(null);
    }
  };

  const handleRemove = async (plate: string, custId: number) => {
    try {
      setActionLoading(plate);
      setError(null);
      setSuccessMsg(null);

      await api.leaveQueue(plate, custId);
      setSuccessMsg(`Vehicle ${plate} was removed from the waiting queue.`);
      loadData();
    } catch (err: any) {
      setError(err.message || 'Failed to remove from queue');
    } finally {
      setActionLoading(null);
    }
  };

  return (
    <div className="space-y-8">
      <div>
        <h1 className="text-2xl font-bold text-white tracking-tight flex items-center gap-2">
          <Clock className="w-6 h-6 text-amber-400" />
          Waiting Queue Management
        </h1>
        <p className="text-sm text-slate-400 mt-1">
          FIFO queue allocation when parking capacity is exhausted. Dequeued automatically upon vehicle exit or admitted into empty slots.
        </p>
      </div>

      {/* Available Slots Notice Banner */}
      {availableSlots > 0 && (
        <div className="p-4 bg-emerald-950/40 border border-emerald-600/60 rounded-xl text-emerald-300 text-sm flex items-center justify-between">
          <div className="flex items-center gap-2.5">
            <Info className="w-5 h-5 text-emerald-400 shrink-0" />
            <span>
              <strong>{availableSlots} parking slots are currently AVAILABLE.</strong> Arriving vehicles can be checked in directly at the Gate Terminal or admitted immediately.
            </span>
          </div>
          {queue.length > 0 && (
            <button
              onClick={() => handleAdmit(queue[0].license_plate)}
              disabled={actionLoading !== null}
              className="px-3 py-1.5 bg-emerald-600 hover:bg-emerald-500 text-white rounded-lg text-xs font-bold transition shadow"
            >
              Admit First Queued Vehicle
            </button>
          )}
        </div>
      )}

      {error && (
        <div className="p-4 bg-rose-950/40 border border-rose-800 rounded-xl text-rose-300 text-sm flex items-center gap-2">
          <AlertCircle className="w-4 h-4 shrink-0" />
          <span>{error}</span>
        </div>
      )}

      {successMsg && (
        <div className="p-4 bg-emerald-950/40 border border-emerald-600/80 rounded-xl text-emerald-300 text-sm flex items-center gap-2">
          <CheckCircle2 className="w-4 h-4 shrink-0" />
          <span>{successMsg}</span>
        </div>
      )}

      <div className="grid grid-cols-1 lg:grid-cols-3 gap-8">
        {/* Join Queue Form */}
        <form
          onSubmit={handleJoinQueue}
          className="p-6 bg-slate-900 border border-slate-800 rounded-2xl space-y-4 h-fit"
        >
          <div className="text-base font-semibold text-white border-b border-slate-800 pb-3 flex items-center gap-2">
            <PlusCircle className="w-4 h-4 text-amber-400" />
            Enqueue Customer
          </div>

          <div>
            <label className="block text-xs font-medium text-slate-400 mb-1">
              Customer
            </label>
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
          </div>

          <div>
            <label className="block text-xs font-medium text-slate-400 mb-1">
              Vehicle Plate *
            </label>
            <input
              type="text"
              required
              placeholder="e.g. MH-12-DE-9988"
              value={queuePlate}
              onChange={(e) => setQueuePlate(e.target.value)}
              className="w-full bg-slate-800 border border-slate-700 rounded-lg px-3 py-2 text-sm text-slate-100 uppercase font-mono focus:outline-none focus:border-indigo-500"
            />
          </div>

          <div>
            <label className="block text-xs font-medium text-slate-400 mb-1">
              Required Category
            </label>
            <select
              value={queueCategory}
              onChange={(e) => setQueueCategory(e.target.value as VehicleCategory)}
              className="w-full bg-slate-800 border border-slate-700 rounded-lg px-3 py-2 text-sm text-slate-100 focus:outline-none focus:border-indigo-500"
            >
              <option value="CAR">Car</option>
              <option value="BIKE">Motorcycle / Bike</option>
              <option value="EV">Electric Vehicle (EV)</option>
            </select>
          </div>

          <button
            type="submit"
            disabled={loading}
            className="w-full py-2.5 bg-amber-600 hover:bg-amber-500 text-white rounded-xl text-sm font-semibold shadow-lg shadow-amber-600/30 transition flex items-center justify-center gap-2 cursor-pointer"
          >
            <Clock className="w-4 h-4" />
            {loading ? 'Adding...' : 'Add to Waiting Queue'}
          </button>
        </form>

        {/* Live FIFO Queue List */}
        <div className="lg:col-span-2 space-y-4">
          <div className="flex items-center justify-between">
            <h2 className="text-base font-semibold text-white">
              FIFO Waiting Line ({queue.length} customers)
            </h2>
            <span className="text-xs text-slate-400 font-mono">
              Managed via C11 Queue Data Structure
            </span>
          </div>

          <div className="overflow-x-auto bg-slate-900 border border-slate-800 rounded-2xl">
            <table className="w-full text-left text-xs">
              <thead className="bg-slate-800/60 text-slate-400 uppercase tracking-wider">
                <tr>
                  <th className="py-3 px-4">Position</th>
                  <th className="py-3 px-4">Customer</th>
                  <th className="py-3 px-4">Contact</th>
                  <th className="py-3 px-4">License Plate</th>
                  <th className="py-3 px-4">Category</th>
                  <th className="py-3 px-4 text-right">Actions</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-slate-800">
                {queue.map((q) => (
                  <tr
                    key={`${q.position}-${q.license_plate}`}
                    className="hover:bg-slate-800/40 transition"
                  >
                    <td className="py-3 px-4 font-bold text-amber-400">
                      #{q.position}
                    </td>
                    <td className="py-3 px-4 font-semibold text-white">
                      {q.customer_name}
                    </td>
                    <td className="py-3 px-4 text-slate-400 font-mono">
                      {q.phone}
                    </td>
                    <td className="py-3 px-4 font-mono font-bold text-indigo-300">
                      {q.license_plate}
                    </td>
                    <td className="py-3 px-4">
                      <span className="px-2 py-0.5 rounded bg-slate-800 text-slate-300">
                        {q.category}
                      </span>
                    </td>
                    <td className="py-3 px-4 text-right">
                      <div className="flex items-center justify-end gap-2">
                        {availableSlots > 0 && (
                          <button
                            type="button"
                            onClick={() => handleAdmit(q.license_plate)}
                            disabled={actionLoading === q.license_plate}
                            className="px-2.5 py-1 bg-emerald-600 hover:bg-emerald-500 text-white rounded-lg text-xs font-semibold flex items-center gap-1 shadow-sm transition"
                            title="Admit to available slot"
                          >
                            <LogIn className="w-3 h-3" />
                            <span>Admit</span>
                          </button>
                        )}
                        <button
                          type="button"
                          onClick={() => handleRemove(q.license_plate, q.customer_id)}
                          disabled={actionLoading === q.license_plate}
                          className="px-2 py-1 bg-slate-800 hover:bg-rose-900/60 text-slate-400 hover:text-rose-300 border border-slate-700 hover:border-rose-700 rounded-lg text-xs transition"
                          title="Remove from queue"
                        >
                          <Trash2 className="w-3 h-3" />
                        </button>
                      </div>
                    </td>
                  </tr>
                ))}
                {queue.length === 0 && (
                  <tr>
                    <td colSpan={6} className="py-8 text-center text-slate-500">
                      Queue is currently empty. All arriving vehicles are accommodated immediately.
                    </td>
                  </tr>
                )}
              </tbody>
            </table>
          </div>
        </div>
      </div>
    </div>
  );
}

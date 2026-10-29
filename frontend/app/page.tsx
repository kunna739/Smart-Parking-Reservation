'use client';

import React, { useEffect, useState } from 'react';
import { api } from '@/lib/api';
import { Facility, Slot } from '@/lib/types';
import SlotGrid from '@/components/SlotGrid';
import SlotDetailModal from '@/components/SlotDetailModal';
import {
  Car,
  Clock,
  DollarSign,
  PieChart,
  ShieldCheck,
  CheckCircle2,
  RefreshCw,
  PlusCircle,
} from 'lucide-react';
import Link from 'next/link';

export default function DashboardPage() {
  const [facility, setFacility] = useState<Facility | null>(null);
  const [slots, setSlots] = useState<Slot[]>([]);
  const [selectedSlot, setSelectedSlot] = useState<Slot | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  const loadData = async () => {
    try {
      setLoading(true);
      setError(null);
      const [facData, slotsData] = await Promise.all([
        api.getFacility(),
        api.getSlots(),
      ]);
      setFacility(facData);
      setSlots(slotsData);
    } catch (err: any) {
      setError(err.message || 'Failed to load facility data');
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    loadData();
    const interval = setInterval(loadData, 5000);
    return () => clearInterval(interval);
  }, []);

  return (
    <div className="space-y-8">
      {/* Header */}
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-4">
        <div>
          <h1 className="text-2xl font-bold text-white tracking-tight">
            Facility Overview & Live Occupancy
          </h1>
          <p className="text-sm text-slate-400 mt-1">
            Real-time telemetry powered by C11 memory engine & SQLite3 persistence
          </p>
        </div>

        <div className="flex items-center gap-3">
          <button
            onClick={loadData}
            disabled={loading}
            className="flex items-center gap-2 px-3 py-2 bg-slate-800 hover:bg-slate-700 text-slate-200 rounded-lg text-xs font-semibold border border-slate-700 transition"
          >
            <RefreshCw className={`w-3.5 h-3.5 ${loading ? 'animate-spin' : ''}`} />
            Refresh
          </button>
          <Link
            href="/reserve"
            className="flex items-center gap-2 px-4 py-2 bg-indigo-600 hover:bg-indigo-500 text-white rounded-lg text-xs font-semibold shadow-lg shadow-indigo-600/30 transition"
          >
            <PlusCircle className="w-3.5 h-3.5" />
            Reserve Slot
          </Link>
        </div>
      </div>

      {error && (
        <div className="p-4 bg-rose-950/40 border border-rose-800 rounded-xl text-rose-300 text-sm">
          {error}
        </div>
      )}

      {/* Metric Cards Grid */}
      <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4">
        {/* Total Capacity */}
        <div className="p-5 bg-slate-900 border border-slate-800 rounded-2xl">
          <div className="flex items-center justify-between text-slate-400 text-xs font-medium">
            <span>Total Capacity</span>
            <Car className="w-4 h-4 text-indigo-400" />
          </div>
          <div className="mt-3 text-3xl font-bold text-white">
            {facility?.total_slots ?? 0}
          </div>
          <div className="mt-2 text-xs text-slate-500">
            Car, Bike & EV Designated Spaces
          </div>
        </div>

        {/* Available Spaces */}
        <div className="p-5 bg-slate-900 border border-slate-800 rounded-2xl">
          <div className="flex items-center justify-between text-slate-400 text-xs font-medium">
            <span>Available Spaces</span>
            <CheckCircle2 className="w-4 h-4 text-emerald-400" />
          </div>
          <div className="mt-3 text-3xl font-bold text-emerald-400">
            {facility?.available_slots ?? 0}
          </div>
          <div className="mt-2 text-xs text-emerald-500/80 font-medium">
            Ready for instant allocation
          </div>
        </div>

        {/* Occupancy Rate */}
        <div className="p-5 bg-slate-900 border border-slate-800 rounded-2xl">
          <div className="flex items-center justify-between text-slate-400 text-xs font-medium">
            <span>Occupancy Rate</span>
            <PieChart className="w-4 h-4 text-amber-400" />
          </div>
          <div className="mt-3 text-3xl font-bold text-amber-400">
            {facility?.occupancy_rate?.toFixed(1) ?? '0.0'}%
          </div>
          <div className="mt-2 text-xs text-slate-500">
            {facility?.occupied_slots ?? 0} Occupied · {facility?.reserved_slots ?? 0} Reserved
          </div>
        </div>

        {/* Rate Schedule */}
        <div className="p-5 bg-slate-900 border border-slate-800 rounded-2xl">
          <div className="flex items-center justify-between text-slate-400 text-xs font-medium">
            <span>Hourly Rates</span>
            <DollarSign className="w-4 h-4 text-emerald-400" />
          </div>
          <div className="mt-2 space-y-1">
            <div className="flex justify-between text-xs">
              <span className="text-slate-400">Car:</span>
              <span className="font-semibold text-slate-200">₹{facility?.rates.car ?? 250}/hr</span>
            </div>
            <div className="flex justify-between text-xs">
              <span className="text-slate-400">Bike:</span>
              <span className="font-semibold text-slate-200">₹{facility?.rates.bike ?? 100}/hr</span>
            </div>
            <div className="flex justify-between text-xs">
              <span className="text-slate-400">EV:</span>
              <span className="font-semibold text-emerald-400">₹{facility?.rates.ev ?? 200}/hr</span>
            </div>
          </div>
        </div>
      </div>

      {/* 2D Parking Slot Interactive Visualizer */}
      <div className="space-y-4">
        <div className="flex items-center justify-between">
          <h2 className="text-lg font-semibold text-white">Live Slot Layout Grid</h2>
          <span className="text-xs text-slate-400 font-mono">
            {slots.length} slots synchronized with in-memory array · Click any slot for details
          </span>
        </div>
        <SlotGrid slots={slots} onSelectSlot={(s) => setSelectedSlot(s)} />
      </div>

      {/* Interactive Slot Details Modal */}
      <SlotDetailModal
        slot={selectedSlot}
        onClose={() => {
          setSelectedSlot(null);
          loadData();
        }}
      />
    </div>
  );
}

'use client';

import React, { useEffect, useState } from 'react';
import { Activity, Search, ShieldCheck, Car, HelpCircle } from 'lucide-react';
import { api } from '@/lib/api';

export default function Navbar() {
  const [isBackendHealthy, setIsBackendHealthy] = useState<boolean | null>(null);
  const [searchPlate, setSearchPlate] = useState('');
  const [searchResult, setSearchResult] = useState<any>(null);
  const [isSearching, setIsSearching] = useState(false);
  const [searchModalOpen, setSearchModalOpen] = useState(false);

  useEffect(() => {
    const checkHealth = async () => {
      try {
        const res = await fetch('/api/health');
        const data = await res.json();
        setIsBackendHealthy(data.success === true);
      } catch {
        setIsBackendHealthy(false);
      }
    };
    checkHealth();
    const timer = setInterval(checkHealth, 10000);
    return () => clearInterval(timer);
  }, []);

  const handleSearch = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!searchPlate.trim()) return;
    setIsSearching(true);
    setSearchResult(null);
    try {
      const data = await api.searchVehicle(searchPlate.trim());
      setSearchResult(data);
    } catch (err: any) {
      setSearchResult({ error: err.message || 'Vehicle not found' });
    } finally {
      setIsSearching(false);
      setSearchModalOpen(true);
    }
  };

  return (
    <>
      <header className="sticky top-0 z-40 flex items-center justify-between h-16 px-6 bg-slate-900/90 backdrop-blur border-b border-slate-800">
        <div className="flex items-center gap-3">
          <div className="p-2 bg-indigo-600 rounded-lg text-white shadow-lg shadow-indigo-500/20">
            <Car className="w-5 h-5" />
          </div>
          <div>
            <span className="font-bold text-lg bg-gradient-to-r from-white to-slate-400 bg-clip-text text-transparent">
              SmartParking OS
            </span>
            <span className="ml-2 text-xs font-medium px-2 py-0.5 rounded bg-indigo-950 text-indigo-400 border border-indigo-800">
              C11 + Next.js
            </span>
          </div>
        </div>

        {/* Global Plate Search Bar */}
        <form onSubmit={handleSearch} className="flex-1 max-w-md mx-8 relative">
          <input
            type="text"
            placeholder="Search vehicle plate (e.g. BAT-001)..."
            value={searchPlate}
            onChange={(e) => setSearchPlate(e.target.value)}
            className="w-full bg-slate-800/80 border border-slate-700 rounded-lg pl-10 pr-4 py-2 text-sm text-slate-100 placeholder-slate-400 focus:outline-none focus:border-indigo-500 focus:ring-1 focus:ring-indigo-500 transition"
          />
          <Search className="absolute left-3 top-2.5 w-4 h-4 text-slate-400" />
        </form>

        <div className="flex items-center gap-4">
          {/* Backend Status Pill */}
          <div className="flex items-center gap-2 px-3 py-1.5 rounded-full text-xs font-medium border bg-slate-800/80 border-slate-700">
            <span
              className={`w-2 h-2 rounded-full ${
                isBackendHealthy === true
                  ? 'bg-emerald-500 animate-pulse'
                  : isBackendHealthy === false
                  ? 'bg-rose-500'
                  : 'bg-amber-500'
              }`}
            />
            <span className="text-slate-300">
              Backend:{' '}
              <strong
                className={
                  isBackendHealthy === true
                    ? 'text-emerald-400'
                    : isBackendHealthy === false
                    ? 'text-rose-400'
                    : 'text-amber-400'
                }
              >
                {isBackendHealthy === true
                  ? 'Online (C11)'
                  : isBackendHealthy === false
                  ? 'Offline'
                  : 'Checking...'}
              </strong>
            </span>
          </div>
        </div>
      </header>

      {/* Quick Search Modal */}
      {searchModalOpen && (
        <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/60 backdrop-blur-sm p-4">
          <div className="bg-slate-900 border border-slate-800 rounded-xl max-w-md w-full p-6 shadow-2xl">
            <div className="flex justify-between items-center mb-4">
              <h3 className="text-lg font-semibold text-white flex items-center gap-2">
                <Search className="w-5 h-5 text-indigo-400" />
                Vehicle Search Result
              </h3>
              <button
                onClick={() => setSearchModalOpen(false)}
                className="text-slate-400 hover:text-white"
              >
                ✕
              </button>
            </div>

            {searchResult?.error ? (
              <div className="p-4 bg-rose-950/40 border border-rose-800 rounded-lg text-rose-300 text-sm">
                {searchResult.error}
              </div>
            ) : searchResult ? (
              <div className="space-y-3">
                <div className="p-3 bg-slate-800/80 rounded-lg flex justify-between items-center">
                  <span className="text-slate-400 text-xs">License Plate:</span>
                  <span className="font-mono font-bold text-white text-base">
                    {searchResult.license_plate}
                  </span>
                </div>
                <div className="p-3 bg-slate-800/80 rounded-lg flex justify-between items-center">
                  <span className="text-slate-400 text-xs">Owner:</span>
                  <span className="font-medium text-slate-200">
                    {searchResult.customer_name}
                  </span>
                </div>
                <div className="p-3 bg-slate-800/80 rounded-lg flex justify-between items-center">
                  <span className="text-slate-400 text-xs">Category:</span>
                  <span className="px-2 py-0.5 rounded text-xs font-semibold bg-indigo-900 text-indigo-200">
                    {searchResult.category}
                  </span>
                </div>
                <div className="p-3 bg-slate-800/80 rounded-lg flex justify-between items-center">
                  <span className="text-slate-400 text-xs">Status:</span>
                  <span
                    className={`px-2 py-0.5 rounded text-xs font-bold ${
                      searchResult.is_parked
                        ? 'bg-emerald-950 text-emerald-400 border border-emerald-800'
                        : 'bg-slate-700 text-slate-300'
                    }`}
                  >
                    {searchResult.is_parked
                      ? `Parked at Slot ${searchResult.slot_number}`
                      : 'Not Currently Inside'}
                  </span>
                </div>
              </div>
            ) : null}

            <button
              onClick={() => setSearchModalOpen(false)}
              className="mt-6 w-full py-2 bg-slate-800 hover:bg-slate-700 text-white rounded-lg text-sm font-medium transition"
            >
              Close
            </button>
          </div>
        </div>
      )}
    </>
  );
}

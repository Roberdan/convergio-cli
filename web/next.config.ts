import type { NextConfig } from "next";

const nextConfig: NextConfig = {
  // Add security headers for proper permissions handling
  async headers() {
    return [
      {
        source: '/:path*',
        headers: [
          {
            // Allow microphone and camera for voice sessions
            key: 'Permissions-Policy',
            value: 'microphone=(self), camera=(self), display-capture=(self)',
          },
          {
            // CORS for API routes
            key: 'Access-Control-Allow-Origin',
            value: '*',
          },
        ],
      },
    ];
  },
};

export default nextConfig;

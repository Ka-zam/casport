import { defineConfig } from 'vite';

export default defineConfig({
  root: '.',
  publicDir: 'public',
  build: {
    outDir: 'dist',
    assetsDir: 'assets',
    rollupOptions: {
      output: {
        // Keep WASM files in root for easier loading
        assetFileNames: (assetInfo) => {
          if (assetInfo.name.endsWith('.wasm')) {
            return '[name][extname]';
          }
          return 'assets/[name]-[hash][extname]';
        }
      }
    }
  },
  server: {
    port: 5173, // Standard Vite port
    open: true,
    headers: {
      'Cross-Origin-Opener-Policy': 'same-origin',
      'Cross-Origin-Embedder-Policy': 'require-corp'
    },
    // Watch for WASM file changes
    watch: {
      ignored: ['!**/public/**']
    }
  },
  assetsInclude: ['**/*.wgsl', '**/*.wasm'],
  // Optimize for development
  optimizeDeps: {
    exclude: ['cascadix_wasm.js'] // Don't pre-bundle WASM module
  }
});
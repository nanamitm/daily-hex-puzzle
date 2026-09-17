// Cross-origin isolation for hosts that cannot send COOP/COEP headers,
// such as GitHub Pages. Qt's multithreaded WebAssembly build needs
// SharedArrayBuffer, which browsers only hand out to isolated pages.
//
// The service worker re-serves every response with the headers added; the
// first visit reloads once, after which the page is cross-origin isolated.

if (typeof window === 'undefined') {
    // ── service worker side ────────────────────────────────────────────────
    self.addEventListener('install', () => self.skipWaiting());
    self.addEventListener('activate', e => e.waitUntil(self.clients.claim()));

    self.addEventListener('fetch', event => {
        const req = event.request;
        if (req.cache === 'only-if-cached' && req.mode !== 'same-origin') return;

        event.respondWith(fetch(req).then(res => {
            if (res.status === 0) return res;   // opaque response, leave as is
            const headers = new Headers(res.headers);
            headers.set('Cross-Origin-Embedder-Policy', 'require-corp');
            headers.set('Cross-Origin-Opener-Policy', 'same-origin');
            headers.set('Cross-Origin-Resource-Policy', 'cross-origin');
            return new Response(res.body, {
                status: res.status, statusText: res.statusText, headers,
            });
        }).catch(err => console.error('coi-serviceworker:', err)));
    });
} else {
    // ── page side ──────────────────────────────────────────────────────────
    const swUrl = document.currentScript.src;

    if (!window.crossOriginIsolated && navigator.serviceWorker) {
        // A fresh worker only controls the page after a reload
        navigator.serviceWorker.addEventListener('controllerchange',
            () => window.location.reload());

        navigator.serviceWorker.register(swUrl)
            .then(() => navigator.serviceWorker.ready)
            .then(() => {
                if (!navigator.serviceWorker.controller) window.location.reload();
            })
            .catch(err => console.error('coi-serviceworker:', err));
    }
}

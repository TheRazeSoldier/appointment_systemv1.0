// ==================== API Configuration ====================
const API_BASE = '';

// ==================== State ====================
let currentUser = null;
let currentToken = null;
let currentProvider = null;
let currentPage = 'home';
let currentPageData = null;
let navigationHistory = [];
let _bookingProviderId = 0;
let _bookingBizHours = '';

// ==================== Helpers ====================
function $(id) { return document.getElementById(id); }

function api(path, options = {}) {
    const headers = { 'Content-Type': 'application/json' };
    if (currentToken) headers['Authorization'] = 'Bearer ' + currentToken;
    return fetch(API_BASE + path, { ...options, headers: { ...headers, ...options.headers } })
        .then(r => r.json().then(data => ({ status: r.status, data })));
}

function formatDate(dateStr) {
    if (!dateStr) return '';
    const d = new Date(dateStr);
    return d.toLocaleDateString('zh-CN', { year: 'numeric', month: '2-digit', day: '2-digit' });
}

function formatDateTime(dateStr) {
    if (!dateStr) return '';
    const d = new Date(dateStr);
    return d.toLocaleString('zh-CN');
}

function showToast(message, type = 'info') {
    const container = $('toastContainer');
    const toast = document.createElement('div');
    toast.className = `toast ${type}`;
    toast.innerHTML = `<span class="toast-message">${message}</span><span class="toast-close" onclick="this.parentElement.remove()">&times;</span>`;
    container.appendChild(toast);
    setTimeout(() => { if (toast.parentElement) toast.remove(); }, 4000);
}

// ==================== Scroll Animations ====================
function scrollToSection(id) {
    const el = document.getElementById(id);
    if (el) el.scrollIntoView({ behavior: 'smooth' });
}

function scrollToTop() {
    window.scrollTo({ top: 0, behavior: 'smooth' });
}

// Navbar scroll effect
let lastScrollY = 0;
function handleNavScroll() {
    const navbar = document.getElementById('navbar');
    const scrollY = window.scrollY;
    if (scrollY > 50) {
        navbar.classList.add('scrolled');
    } else {
        navbar.classList.remove('scrolled');
    }
    lastScrollY = scrollY;
}

// Active nav link tracking
function updateActiveNavLink() {
    const sections = document.querySelectorAll('section[id]');
    const scrollY = window.scrollY + 100;
    let current = '';
    sections.forEach(section => {
        const sectionTop = section.offsetTop;
        const sectionHeight = section.offsetHeight;
        if (scrollY >= sectionTop && scrollY < sectionTop + sectionHeight) {
            current = section.getAttribute('id');
        }
    });
    document.querySelectorAll('.nav-link').forEach(link => {
        link.classList.toggle('active', link.getAttribute('href') === '#' + current);
    });
}

// Scroll progress bar
function updateScrollProgress() {
    const winScroll = document.documentElement.scrollTop;
    const height = document.documentElement.scrollHeight - document.documentElement.clientHeight;
    const scrolled = height > 0 ? (winScroll / height) * 100 : 0;
    const progressBar = document.getElementById('scrollProgress');
    if (progressBar) progressBar.style.width = scrolled + '%';
}

// Back to top visibility
function updateBackToTop() {
    const btn = document.getElementById('backToTop');
    if (!btn) return;
    if (window.scrollY > 500) {
        btn.classList.add('visible');
    } else {
        btn.classList.remove('visible');
    }
}

// ==================== Bidirectional Scroll Reveal ====================
function initScrollReveal() {
    const revealElements = document.querySelectorAll('.reveal, .reveal-left, .reveal-right, .reveal-scale');
    
    const observer = new IntersectionObserver((entries) => {
        entries.forEach(entry => {
            if (entry.isIntersecting) {
                entry.target.classList.add('visible');
            } else {
                // Remove class when scrolling back up past the element
                entry.target.classList.remove('visible');
            }
        });
    }, { threshold: 0.15, rootMargin: '0px 0px -60px 0px' });

    revealElements.forEach(el => observer.observe(el));
}

// ==================== Parallax Effect ====================
function handleParallax() {
    const scrollY = window.scrollY;
    const parallaxElements = document.querySelectorAll('.parallax-slow');
    
    parallaxElements.forEach(el => {
        const speed = parseFloat(el.getAttribute('data-parallax')) || 0.1;
        const offset = scrollY * speed;
        el.style.transform = `translateY(${offset}px)`;
    });
}

// Counter animation
function animateCounters() {
    const counters = document.querySelectorAll('.counter');
    const observer = new IntersectionObserver((entries) => {
        entries.forEach(entry => {
            if (entry.isIntersecting) {
                const counter = entry.target;
                const target = parseFloat(counter.getAttribute('data-target'));
                const duration = 2000;
                const start = performance.now();

                function update(now) {
                    const elapsed = now - start;
                    const progress = Math.min(elapsed / duration, 1);
                    const eased = 1 - Math.pow(1 - progress, 3);
                    const current = target * eased;
                    counter.textContent = target >= 100 ? Math.floor(current).toLocaleString() : current.toFixed(1);
                    if (progress < 1) requestAnimationFrame(update);
                }
                requestAnimationFrame(update);
                observer.unobserve(counter);
            }
        });
    }, { threshold: 0.5 });
    counters.forEach(c => observer.observe(c));
}

// ==================== Navigation ====================
function navigateToDefaultPage() {
    // 登录后永远不回到公共首页
    if (currentUser) {
        navigate(currentProvider ? 'providerDashboard' : 'dashboard');
    } else {
        navigate('home');
    }
}

function navigate(page, data) {
    // Record history before changing
    if (currentPage && currentPage !== page) {
        navigationHistory.push({ page: currentPage, data: currentPageData });
    }
    currentPage = page;
    currentPageData = data;

    document.querySelectorAll('.page').forEach(p => p.classList.remove('active'));
    const pageEl = document.getElementById('page-' + page);
    if (pageEl) pageEl.classList.add('active');

    window.scrollTo(0, 0);

    switch(page) {
        case 'home': loadHomePage(); break;
        case 'dashboard': loadDashboard(); break;
        case 'services': loadServices(); break;
        case 'serviceDetail': loadServiceDetail(data); break;
        case 'providers': loadProviders(); break;
        case 'myAppointments': loadMyAppointments(); break;
        case 'notifications': loadNotifications(); break;
        case 'profile': loadProfile(); break;
        case 'coupons': loadUserCoupons(); break;
        case 'myApplication': loadMyApplication(); break;
        case 'adminDashboard': loadAdminDashboard(); break;
        case 'providerDashboard': loadProviderDashboard(); break;
        case 'reports': loadReports(); break;
    }
}

function goBack() {
    if (navigationHistory.length > 0) {
        const prev = navigationHistory.pop();
        // Navigate without recording history to avoid loops
        const prevPage = prev.page;
        const prevData = prev.data;
        currentPage = prevPage;
        currentPageData = prevData;

        document.querySelectorAll('.page').forEach(p => p.classList.remove('active'));
        const pageEl = document.getElementById('page-' + prevPage);
        if (pageEl) pageEl.classList.add('active');
        const homePage = document.getElementById('page-home');
        if (homePage) homePage.classList.toggle('active', prevPage === 'home');
        window.scrollTo(0, 0);

        switch(prevPage) {
            case 'home': loadHomePage(); break;
            case 'dashboard': loadDashboard(); break;
            case 'services': loadServices(); break;
            case 'serviceDetail': loadServiceDetail(prevData); break;
            case 'providers': loadProviders(); break;
            case 'myAppointments': loadMyAppointments(); break;
            case 'notifications': loadNotifications(); break;
            case 'profile': loadProfile(); break;
            case 'coupons': loadUserCoupons(); break;
            case 'myApplication': loadMyApplication(); break;
            case 'adminDashboard': loadAdminDashboard(); break;
            case 'providerDashboard': loadProviderDashboard(); break;
        }
    } else {
        // 登录后兜底回到默认页面，不回到公共首页
        navigateToDefaultPage();
    }
}

// ==================== Auth ====================
function handleRegister(e) {
    e.preventDefault();
    const errorEl = $('registerError');
    errorEl.textContent = '';
    
    const data = {
        username: $('regUsername').value.trim(),
        email: $('regEmail').value.trim(),
        password: $('regPassword').value,
        phone: $('regPhone').value.trim(),
        role: $('regRole').value
    };
    
    api('/api/auth/register', { method: 'POST', body: JSON.stringify(data) }).then(({ status, data: resp }) => {
        if (status === 200) {
            currentToken = resp.token;
            currentUser = resp.user;
            localStorage.setItem('token', resp.token);
            localStorage.setItem('user', JSON.stringify(resp.user));
            closeModal('registerModal');
            updateNavState();
            showToast('注册成功！欢迎加入', 'success');
            navigate('dashboard');
        } else {
            errorEl.textContent = resp.error || '注册失败';
        }
    });
}

function handleLogin(e) {
    e.preventDefault();
    const errorEl = $('loginError');
    errorEl.textContent = '';
    
    const data = {
        username: $('loginUsername').value.trim(),
        password: $('loginPassword').value
    };
    
    api('/api/auth/login', { method: 'POST', body: JSON.stringify(data) }).then(({ status, data: resp }) => {
        if (status === 200) {
            currentToken = resp.token;
            currentUser = resp.user;
            if (resp.provider) currentProvider = resp.provider;
            localStorage.setItem('token', resp.token);
            localStorage.setItem('user', JSON.stringify(resp.user));
            if (resp.provider) localStorage.setItem('provider', JSON.stringify(resp.provider));
            closeModal('loginModal');
            updateNavState();
            showToast('登录成功！', 'success');
            navigate(currentProvider ? 'providerDashboard' : 'dashboard');
        } else {
            errorEl.textContent = resp.error || '登录失败';
        }
    });
}

function logout() {
    currentToken = null; currentUser = null; currentProvider = null;
    localStorage.removeItem('token'); localStorage.removeItem('user'); localStorage.removeItem('provider');
    updateNavState();
    showToast('已退出登录', 'info');
    navigate('home');
}

function updateNavState() {
    const navActions = $('navActions');
    const navUser = $('navUser');
    const userNameDisplay = $('userNameDisplay');
    const userAvatar = $('userAvatar');
    const providerLink = $('providerLink');
    const navProviderLink = $('navProviderLink');
    const adminLink = $('adminLink');
    const appLink = $('appLink');
    const providerFeatureCard = $('providerFeatureCard');
    
    if (currentUser) {
        navActions.style.display = 'none';
        navUser.style.display = 'flex';
        userNameDisplay.textContent = currentUser.username;
        userAvatar.textContent = currentUser.username.charAt(0).toUpperCase();
        
        if (currentUser.role === 'admin') {
            if (adminLink) adminLink.style.display = 'block';
            if (providerLink) providerLink.style.display = 'block';
            if (navProviderLink) navProviderLink.style.display = 'none';
            if (appLink) appLink.style.display = 'none';
            if (providerFeatureCard) providerFeatureCard.style.display = 'none';
        } else if (currentUser.role === 'provider') {
            if (adminLink) adminLink.style.display = 'none';
            if (providerLink) providerLink.style.display = 'block';
            if (navProviderLink) navProviderLink.style.display = 'inline-block';
            if (appLink) appLink.style.display = 'none';
            if (providerFeatureCard) providerFeatureCard.style.display = 'flex';
        } else {
            if (adminLink) adminLink.style.display = 'none';
            if (providerLink) providerLink.style.display = 'none';
            if (navProviderLink) navProviderLink.style.display = 'none';
            if (appLink) appLink.style.display = 'block';
            if (providerFeatureCard) providerFeatureCard.style.display = 'none';
        }
        loadUnreadCount();
    } else {
        navActions.style.display = 'flex';
        navUser.style.display = 'none';
        if (providerLink) providerLink.style.display = 'none';
        if (navProviderLink) navProviderLink.style.display = 'none';
        if (adminLink) adminLink.style.display = 'none';
        if (appLink) appLink.style.display = 'none';
        if (providerFeatureCard) providerFeatureCard.style.display = 'none';
    }
}

function initAuth() {
    const token = localStorage.getItem('token');
    const user = localStorage.getItem('user');
    const provider = localStorage.getItem('provider');
    if (token && user) {
        currentToken = token;
        currentUser = JSON.parse(user);
        if (provider) currentProvider = JSON.parse(provider);
        updateNavState();
        return true;
    }
    return false;
}

// ==================== Modal Management ====================
function showModal(id) {
    if (!currentUser && (id === 'appointmentModal' || id === 'reviewModal' || id === 'providerRegisterModal' || id === 'addServiceModal')) {
        showToast('请先登录', 'warning');
        showModal('loginModal');
        return;
    }
    document.getElementById(id).classList.add('show');
}

function closeModal(id) { document.getElementById(id).classList.remove('show'); }
function switchModal(from, to) { closeModal(from); showModal(to); }
function toggleDropdown(id) { document.getElementById(id).classList.toggle('show'); }
function toggleMobileMenu() { document.getElementById('navLinks').classList.toggle('show'); }

document.addEventListener('click', function(e) {
    if (!e.target.closest('.user-dropdown')) {
        document.querySelectorAll('.dropdown-menu').forEach(m => m.classList.remove('show'));
    }
    if (e.target.classList.contains('modal')) {
        e.target.classList.remove('show');
    }
});

// ==================== Contact Form ====================
function handleContactSubmit(e) {
    e.preventDefault();
    const name = $('contactName').value.trim();
    const phone = $('contactPhone').value.trim();
    const email = $('contactEmail').value.trim();
    const type = $('contactType').value;
    const message = $('contactMessage').value.trim();
    
    showToast(`感谢 ${name}，您的咨询已提交，我们将尽快与您联系！`, 'success');
    e.target.reset();
}

// ==================== Home Page ====================
function loadHomePage() {
    // Load featured services
    api('/api/services').then(({ data }) => {
        const container = $('featuredServices');
        if (container && data.services && data.services.length > 0) {
            container.innerHTML = data.services.slice(0, 6).map(s => `
                <div class="service-card reveal" onclick="navigate('serviceDetail', ${s.id})">
                    <div class="service-card-image ${getCategoryClass(s.category)}">
                        <span class="icon-text">${getCategoryIcon(s.category)}</span>
                    </div>
                    <div class="service-card-body">
                        <h3>${escHtml(s.name)}</h3>
                        <p class="service-provider">${escHtml(s.provider_name || '')}</p>
                        <p class="service-desc">${escHtml(s.description).substring(0, 60)}</p>
                        <div class="service-card-meta">
                            <span class="service-price">${s.price > 0 ? '¥' + s.price : '免费'}</span>
                            <span class="service-duration">${s.duration}分钟</span>
                            <span class="service-rating">★ ${(s.avg_rating || 0).toFixed(1)}</span>
                        </div>
                    </div>
                </div>
            `).join('');
            initScrollReveal();
        }
    });
}

// ==================== Dashboard (Logged-in Home) ====================
let dashCategory = '';

function loadDashboard() {
    if (!currentUser) {
        navigate('home');
        return;
    }

    const nameEl = document.getElementById('dashUserName');
    if (nameEl) nameEl.textContent = currentUser.username;

    const searchInput = document.getElementById('dashSearch');
    if (searchInput) searchInput.value = '';

    loadPromoServices();
    loadRecommendedServices();
    loadDashHotServices();
    initScrollReveal();
}

function loadPromoServices() {
    const container = document.getElementById('promoServices');
    if (!container) return;
    container.innerHTML = '<div class="loading">加载中</div>';

    const params = new URLSearchParams();
    if (dashCategory) params.append('category', dashCategory);

    api('/api/services?' + params.toString()).then(({ data }) => {
        if (data.services && data.services.length > 0) {
            // Simulate promo: pick 3 services, add fake discount
            const promos = data.services.slice(0, 3).map((s, i) => ({
                ...s,
                originalPrice: s.price > 0 ? (s.price * (1.2 + i * 0.15)).toFixed(2) : 0,
                discount: [20, 30, 25][i]
            }));
            container.innerHTML = promos.map(s => `
                <div class="promo-card reveal" onclick="navigate('serviceDetail', ${s.id})">
                    <div class="promo-card-image ${getCategoryClass(s.category)}">
                        <span class="promo-tag">${s.discount}% OFF</span>
                        <span class="icon-text">${getCategoryIcon(s.category)}</span>
                    </div>
                    <div class="promo-card-body">
                        <h3>${escHtml(s.name)}</h3>
                        <p class="promo-provider">${escHtml(s.provider_name || '')}</p>
                        <p class="promo-desc">${escHtml(s.description)}</p>
                    </div>
                    <div class="promo-card-footer">
                        <div class="promo-price">
                            <span class="current">${s.price > 0 ? '¥' + s.price : '免费'}</span>
                            ${s.originalPrice > 0 ? `<span class="original">¥${s.originalPrice}</span>` : ''}
                        </div>
                        <span class="promo-discount">-${s.discount}%</span>
                    </div>
                </div>
            `).join('');
            initScrollReveal();
        } else {
            container.innerHTML = '<div class="empty-state"><div class="empty-icon">🏷</div><h3>暂无优惠服务</h3></div>';
        }
    });
}

function loadRecommendedServices() {
    const container = document.getElementById('dashRecommendedServices');
    if (!container) return;
    container.innerHTML = '<div class="loading">加载中</div>';

    api('/api/recommend/services').then(({ data }) => {
        if (data.services && data.services.length > 0) {
            container.innerHTML = data.services.map(s => `
                <div class="service-card reveal" onclick="navigate('serviceDetail', ${s.id})">
                    <div class="service-card-image ${getCategoryClass(s.category)}">
                        <span class="icon-text">${getCategoryIcon(s.category)}</span>
                        ${s.has_coupon ? '<span class="coupon-badge">券</span>' : ''}
                    </div>
                    <div class="service-card-body">
                        <h3>${escHtml(s.name)}</h3>
                        <p class="service-provider">${escHtml(s.provider_name || '')}</p>
                        <p class="service-desc">${escHtml(s.description).substring(0, 60)}</p>
                        <div class="service-card-meta">
                            <span class="service-price">${s.price > 0 ? '¥' + s.price : '免费'}</span>
                            <span class="service-duration">${s.duration}分钟</span>
                            <span class="service-rating">★ ${(s.avg_rating || 0).toFixed(1)}</span>
                        </div>
                    </div>
                </div>
            `).join('');
            initScrollReveal();
        } else {
            container.innerHTML = '<div class="empty-state"><div class="empty-icon">✨</div><h3>暂无推荐服务</h3></div>';
        }
    });
}

function loadDashHotServices() {
    const container = document.getElementById('dashHotServices');
    if (!container) return;
    container.innerHTML = '<div class="loading">加载中</div>';

    api('/api/recommend/hot').then(({ data }) => {
        if (data.services && data.services.length > 0) {
            const hot = data.services.slice(0, 6);
            container.innerHTML = hot.map(s => `
                <div class="service-card reveal" onclick="navigate('serviceDetail', ${s.id})">
                    <div class="service-card-image ${getCategoryClass(s.category)}">
                        <span class="icon-text">${getCategoryIcon(s.category)}</span>
                    </div>
                    <div class="service-card-body">
                        <h3>${escHtml(s.name)}</h3>
                        <p class="service-provider">${escHtml(s.provider_name || '')}</p>
                        <p class="service-desc">${escHtml(s.description).substring(0, 60)}</p>
                        <div class="service-card-meta">
                            <span class="service-price">${s.price > 0 ? '¥' + s.price : '免费'}</span>
                            <span class="service-duration">${s.duration}分钟</span>
                            <span class="service-rating">★ ${(s.avg_rating || 0).toFixed(1)}</span>
                        </div>
                    </div>
                </div>
            `).join('');
            initScrollReveal();
        } else {
            container.innerHTML = '<div class="empty-state"><div class="empty-icon">🔥</div><h3>暂无热门服务</h3></div>';
        }
    });
}

function performDashSearch() {
    const keyword = document.getElementById('dashSearch')?.value.trim();
    if (keyword) {
        navigate('services');
        // Set the search input on the services page
        setTimeout(() => {
            const svcSearch = document.getElementById('serviceSearch');
            if (svcSearch) {
                svcSearch.value = keyword;
                loadServices();
            }
        }, 100);
    }
}

function filterDashCategory(category, btn) {
    dashCategory = category;
    document.querySelectorAll('.quick-cat').forEach(c => c.classList.remove('active'));
    if (btn) btn.classList.add('active');
    loadPromoServices();
    loadRecommendedServices();
    loadDashHotServices();
}

function clearFilters() {
    $('priceMin').value = '';
    $('priceMax').value = '';
    $('durationMin').value = '';
    $('durationMax').value = '';
    $('sortBy').value = '';
    $('sortOrder').value = 'asc';
    loadServices();
}

// ==================== Services ====================
function loadServices() {
    const keyword = $('serviceSearch')?.value || '';
    const category = $('serviceCategoryFilter')?.value || '';
    const minPrice = $('priceMin')?.value || '';
    const maxPrice = $('priceMax')?.value || '';
    const minDuration = $('durationMin')?.value || '';
    const maxDuration = $('durationMax')?.value || '';
    const sortBy = $('sortBy')?.value || '';
    const sortOrder = $('sortOrder')?.value || '';
    
    const params = new URLSearchParams();
    if (keyword) params.append('keyword', keyword);
    if (category) params.append('category', category);
    if (minPrice) params.append('min_price', minPrice);
    if (maxPrice) params.append('max_price', maxPrice);
    if (minDuration) params.append('min_duration', minDuration);
    if (maxDuration) params.append('max_duration', maxDuration);
    if (sortBy) params.append('sort_by', sortBy);
    if (sortOrder) params.append('sort_order', sortOrder);
    
    const container = document.getElementById('allServices');
    if (!container) return;
    container.innerHTML = '<div class="loading">加载中</div>';
    
    api('/api/services/search?' + params.toString()).then(({ data }) => {
        const services = Array.isArray(data) ? data : (data.services || []);
        if (services.length > 0) {
            container.innerHTML = services.map(s => `
                <div class="service-card" onclick="navigate('serviceDetail', ${s.id})">
                    <div class="service-card-image ${getCategoryClass(s.category)}">
                        <span class="icon-text">${getCategoryIcon(s.category)}</span>
                        ${s.has_coupon ? '<span class="coupon-badge">券</span>' : ''}
                    </div>
                    <div class="service-card-body">
                        <h3>${escHtml(s.name)}</h3>
                        <p class="service-provider">${escHtml(s.provider_name || '')}</p>
                        <p class="service-desc">${escHtml(s.description).substring(0, 80)}</p>
                        <div class="service-card-meta">
                            <span class="service-price">${s.price > 0 ? '¥' + s.price : '免费'}</span>
                            <span class="service-duration">${s.duration}分钟</span>
                            <span class="service-rating">★ ${(s.avg_rating || 0).toFixed(1)}</span>
                        </div>
                    </div>
                </div>
            `).join('');
        } else {
            container.innerHTML = '<div class="empty-state"><div class="empty-icon">🔍</div><h3>没有找到服务</h3><p>试试其他搜索条件</p></div>';
        }
    });
}

function loadServiceDetail(serviceId) {
    const container = $('serviceDetailContent');
    container.innerHTML = '<div class="loading">加载中</div>';
    
    Promise.all([
        api('/api/services/' + serviceId),
        api('/api/providers')
    ]).then(([{ data: svc }, { data: providers }]) => {
        if (svc && svc.id) {
            const s = svc;
            const allProviders = Array.isArray(providers) ? providers : (providers.providers || []);
            const p = allProviders.find(pr => pr.id === s.provider_id) || { name: s.provider_name || '未知', category: s.category || '', description: '' };
            _bookingProviderId = s.provider_id;
            _bookingBizHours = p.business_hours || '';
            container.innerHTML = `
                <div class="service-detail">
                    <button class="page-back" onclick="goBack()">← 返回</button>
                    <div class="service-detail-header">
                        <h1>${escHtml(s.name)}</h1>
                        <p style="color:var(--mid-gray);">${escHtml(s.description)}</p>
                        <div class="service-detail-info">
                            <div><span class="label">价格</span><span class="value" style="color:var(--orange);">${s.price > 0 ? '¥' + s.price : '免费'}</span></div>
                            <div><span class="label">时长</span><span class="value">${s.duration} 分钟</span></div>
                            <div><span class="label">分类</span><span class="value">${escHtml(s.category)}</span></div>
                            <div><span class="label">评分</span><span class="value" style="color:var(--orange);">★ ${(s.rating || 0).toFixed(1)}</span></div>
                        </div>
                        <button class="btn btn-primary" style="margin-top:20px;background:var(--orange);" onclick="openAppointmentModal(${s.id}, '${escHtml(s.name)}')">立即预约</button>
                    </div>
                    <div class="detail-section">
                        <h2>${escHtml(p.name)}</h2>
                        <div style="display:flex;align-items:center;gap:16px;">
                            <div class="profile-avatar-lg" style="width:56px;height:56px;font-size:1.5rem;">${p.name.charAt(0)}</div>
                            <div>
                                <h3 style="margin-bottom:4px;">${escHtml(p.name)}</h3>
                                <span class="profile-role">${escHtml(p.category)}</span>
                                <p style="margin-top:4px;color:var(--mid-gray);">${escHtml(p.description)}</p>
                            </div>
                        </div>
                    </div>
                    <div class="detail-section">
                        <h2>用户评价 (${s.reviews ? s.reviews.length : 0})</h2>
                        ${s.reviews && s.reviews.length > 0 ? s.reviews.map(r => `
                            <div style="padding:16px 0;border-bottom:1px solid var(--light-gray);">
                                <div style="display:flex;justify-content:space-between;margin-bottom:8px;">
                                    <span style="font-weight:600;">${escHtml(r.username)}</span>
                                    <span style="color:var(--orange);">${'★'.repeat(r.rating)}${'☆'.repeat(5-r.rating)}</span>
                                </div>
                                <p style="color:var(--mid-gray);">${escHtml(r.comment) || '用户未留下评论'}</p>
                                <span style="font-size:0.8rem;color:var(--mid-gray);">${formatDateTime(r.created_at)}</span>
                            </div>
                        `).join('') : '<p style="color:var(--mid-gray);">暂无评价</p>'}
                    </div>
                </div>
            `;
            document.getElementById('page-serviceDetail').classList.add('active');
        }
    });
}

// ==================== Providers ====================
function loadProviders() {
    const category = $('providerCategoryFilter')?.value || '';
    const dayOfWeek = $('providerDayFilter')?.value || '';
    const params = new URLSearchParams();
    if (category) params.append('category', category);
    if (dayOfWeek) params.append('business_hours', dayOfWeek);
    
    const container = document.getElementById('allProviders');
    if (!container) return;
    container.innerHTML = '<div class="loading">加载中</div>';
    
    api('/api/providers?' + params.toString()).then(({ data }) => {
        const list = Array.isArray(data) ? data : (data.providers || []);
        if (list.length > 0) {
            container.innerHTML = list.map(p => `
                <div class="service-card" onclick="navigate('providerDetail', ${p.id});">
                    <div class="service-card-image ${getCategoryClass(p.category)}">
                        <span class="icon-text">${p.name.charAt(0)}</span>
                    </div>
                    <div class="service-card-body">
                        <h3>${escHtml(p.name)}</h3>
                        <span class="profile-role">${escHtml(p.category)}</span>
                        <p class="service-desc" style="margin-top:8px;">${escHtml(p.description) || '暂无简介'}</p>
                        <p style="color:var(--mid-gray);margin-top:8px;">📌 ${escHtml(p.address)}</p>
                        ${p.business_hours ? `<p style="color:var(--green);margin-top:4px;font-size:0.8rem;">🕐 今日营业</p>` : ''}
                    </div>
                </div>
            `).join('');
        } else {
            container.innerHTML = '<div class="empty-state"><div class="empty-icon">👤</div><h3>暂无服务商</h3></div>';
        }
    });
}

// ==================== Appointments ====================
function openAppointmentModal(serviceId, serviceName) {
    if (!currentUser) { showToast('请先登录', 'warning'); showModal('loginModal'); return; }
    $('apptServiceId').value = serviceId;
    $('apptServiceName').value = serviceName;
    $('apptDate').value = '';
    $('apptDate').min = new Date().toISOString().split('T')[0];
    $('apptTime').value = '';
    $('apptNotes').value = '';
    $('appointmentError').textContent = '';
    $('apptTime').innerHTML = '<option value="">请先选择日期</option>';
    $('apptDate').onchange = function() { updateAvailableTimes(); };
    showModal('appointmentModal');
}

function updateAvailableTimes() {
    const dateStr = $('apptDate').value;
    const timeSelect = $('apptTime');
    if (!dateStr || !_bookingProviderId) {
        timeSelect.innerHTML = '<option value="">请选择日期</option>';
        return;
    }
    timeSelect.innerHTML = '<option value="">加载中...</option>';

    const dayNames = ['周日','周一','周二','周三','周四','周五','周六'];
    const date = new Date(dateStr + 'T00:00:00');
    const dayName = dayNames[date.getDay()];

    let bizHours = {};
    try { bizHours = JSON.parse(_bookingBizHours); } catch(e) {}
    const range = bizHours[dayName];
    if (!range) {
        timeSelect.innerHTML = '<option value="">今日不营业</option>';
        return;
    }

    const parts = range.split('-');
    if (parts.length < 2) {
        timeSelect.innerHTML = '<option value="">营业时间格式错误</option>';
        return;
    }
    const [startH, startM] = parts[0].split(':').map(Number);
    const [endH, endM] = parts[1].split(':').map(Number);
    const startMinutes = startH * 60 + startM;
    const endMinutes = endH * 60 + endM;

    api('/api/appointments/availability?provider_id=' + _bookingProviderId + '&date=' + dateStr).then(({ data }) => {
        const booked = Array.isArray(data) ? data.map(a => a.time) : [];
        timeSelect.innerHTML = '<option value="">请选择时间</option>';
        let h = Math.floor(startMinutes / 60);
        let m = startMinutes % 60;
        let hasSlots = false;
        while (h * 60 + m < endMinutes) {
            const time = `${String(h).padStart(2,'0')}:${String(m).padStart(2,'0')}`;
            if (!booked.includes(time)) {
                timeSelect.innerHTML += `<option value="${time}">${time}</option>`;
                hasSlots = true;
            }
            m += 30;
            if (m >= 60) { h++; m -= 60; }
        }
        if (!hasSlots) {
            timeSelect.innerHTML = '<option value="">该日已约满</option>';
        }
    }).catch(() => {
        timeSelect.innerHTML = '<option value="">加载失败</option>';
    });
}

function handleBookAppointment(e) {
    e.preventDefault();
    const errorEl = $('appointmentError');
    errorEl.textContent = '';
    
    const data = {
        service_id: parseInt($('apptServiceId').value),
        appointment_date: $('apptDate').value,
        appointment_time: $('apptTime').value,
        notes: $('apptNotes').value
    };
    
    api('/api/appointments', { method: 'POST', body: JSON.stringify(data) }).then(({ status, data: resp }) => {
        if (status === 200) { closeModal('appointmentModal'); showToast('预约成功！等待服务商确认', 'success'); }
        else { errorEl.textContent = resp.error || '预约失败(状态码:' + status + ')'; }
    }).catch(err => {
        errorEl.textContent = '网络错误: ' + (err.message || '未知错误');
    });
}

function loadMyAppointments() {
    if (!currentUser) { showToast('请先登录', 'warning'); showModal('loginModal'); return; }
    const container = $('myAppointmentsList');
    container.innerHTML = '<div class="loading">加载中</div>';
    
    api('/api/appointments').then(({ data }) => {
        if (data.appointments && data.appointments.length > 0) {
            container.innerHTML = data.appointments.map(a => `
                <div class="appointment-card">
                    <div class="appointment-header">
                        <span class="appointment-title">${escHtml(a.service_name)}</span>
                        <span class="status-badge status-${a.status}">${getStatusText(a.status)}</span>
                    </div>
                    <div class="appointment-info">
                        <span>📅 ${a.appointment_date} ${a.appointment_time}</span>
                        <span>👤 ${escHtml(a.provider_name)}</span>
                        ${currentUser.role === 'provider' ? `<span>用户: ${escHtml(a.user_name)}</span>` : ''}
                        <span>💰 ¥${a.service_price}</span>
                    </div>
                    ${a.notes ? `<p style="color:var(--mid-gray);margin-bottom:8px;">备注: ${escHtml(a.notes)}</p>` : ''}
                    <div class="appointment-actions">
                        ${a.status === 'pending' && currentUser.role === 'provider' ? `<button class="btn btn-primary btn-sm" style="background:var(--blue);" onclick="confirmAppointment(${a.id})">确认预约</button>` : ''}
                        ${a.status === 'confirmed' && currentUser.role === 'provider' ? `<button class="btn btn-primary btn-sm" style="background:var(--green);" onclick="completeAppointment(${a.id})">完成服务</button>` : ''}
                        ${(a.status === 'pending' || a.status === 'confirmed') ? `<button class="btn btn-primary btn-sm" style="background:#EF4444;" onclick="cancelAppointment(${a.id})">取消预约</button>` : ''}
                        ${a.status === 'completed' && currentUser.role === 'user' ? `<button class="btn btn-outline btn-sm" style="color:var(--orange);border-color:var(--orange);" onclick="openReviewModal(${a.id})">评价</button>` : ''}
                    </div>
                </div>
            `).join('');
        } else {
            container.innerHTML = '<div class="empty-state"><div class="empty-icon">📅</div><h3>暂无预约</h3><p>去浏览服务并预约吧！</p></div>';
        }
    });
}

function cancelAppointment(id) {
    if (!confirm('确定要取消这个预约吗？')) return;
    api('/api/appointments/' + id + '/cancel', { method: 'POST' }).then(({ status, data }) => {
        if (status === 200) { showToast('预约已取消', 'success'); loadMyAppointments(); }
        else { showToast(data.error || '取消失败', 'error'); }
    });
}

function confirmAppointment(id) {
    api('/api/appointments/' + id + '/confirm', { method: 'POST' }).then(({ status, data }) => {
        if (status === 200) { showToast('预约已确认', 'success'); loadMyAppointments(); }
        else { showToast(data.error || '确认失败', 'error'); }
    });
}

function completeAppointment(id) {
    api('/api/appointments/' + id + '/complete', { method: 'POST' }).then(({ status, data }) => {
        if (status === 200) { showToast('服务已完成', 'success'); loadMyAppointments(); }
        else { showToast(data.error || '操作失败', 'error'); }
    });
}

// ==================== Reviews ====================
let currentRating = 5;
function setRating(rating) {
    currentRating = rating;
    $('reviewRating').value = rating;
    const stars = document.querySelectorAll('#starRating span');
    stars.forEach((s, i) => {
        s.innerHTML = i < rating ? '★' : '☆';
        s.classList.toggle('active', i < rating);
    });
}

function openReviewModal(apptId) {
    $('reviewApptId').value = apptId;
    $('reviewComment').value = '';
    setRating(5);
    $('reviewError').textContent = '';
    showModal('reviewModal');
}

function handleSubmitReview(e) {
    e.preventDefault();
    const errorEl = $('reviewError');
    errorEl.textContent = '';
    const data = { appointment_id: parseInt($('reviewApptId').value), rating: currentRating, comment: $('reviewComment').value };
    api('/api/reviews', { method: 'POST', body: JSON.stringify(data) }).then(({ status, data: resp }) => {
        if (status === 200) { closeModal('reviewModal'); showToast('评价提交成功！', 'success'); loadMyAppointments(); }
        else { errorEl.textContent = resp.error || '评价失败'; }
    });
}

// ==================== Notifications ====================
function loadNotifications() {
    if (!currentUser) { showToast('请先登录', 'warning'); showModal('loginModal'); return; }
    const container = $('notificationsList');
    container.innerHTML = '<div class="loading">加载中</div>';
    api('/api/notifications').then(({ data }) => {
        if (data.notifications && data.notifications.length > 0) {
            container.innerHTML = data.notifications.map(n => `
                <div class="notification-card ${n.is_read ? '' : 'unread'}" onclick="markNotifRead(${n.id})">
                    <div class="notification-title">${escHtml(n.title)}</div>
                    <div class="notification-message">${escHtml(n.message)}</div>
                    <div class="notification-time">${formatDateTime(n.created_at)}</div>
                </div>
            `).join('');
        } else {
            container.innerHTML = '<div class="empty-state"><div class="empty-icon">🔔</div><h3>暂无消息</h3></div>';
        }
    });
}

function markNotifRead(id) { api('/api/notifications/' + id + '/read', { method: 'PUT' }).then(() => loadUnreadCount()); }

function loadUnreadCount() {
    if (!currentUser) return;
    api('/api/notifications').then(({ data }) => {
        const badge = $('notifBadge');
        const badgeMenu = $('notifBadgeMenu');
        if (data.unread_count > 0) { badge.style.display = 'inline-flex'; badge.textContent = data.unread_count; badgeMenu.textContent = data.unread_count; }
        else { badge.style.display = 'none'; badgeMenu.textContent = '0'; }
    });
}

// ==================== Profile ====================
function loadProfile() {
    if (!currentUser) { showToast('请先登录', 'warning'); showModal('loginModal'); return; }
    const container = $('profileContent');
    api('/api/auth/profile').then(({ data }) => {
        if (data.user) {
            const u = data.user;
            container.innerHTML = `
                <div class="profile-card">
                    <div class="profile-header">
                        <div class="profile-avatar-lg">${u.username.charAt(0).toUpperCase()}</div>
                        <div>
                            <div class="profile-name">${escHtml(u.username)}</div>
                            <span class="profile-role">${u.role === 'provider' ? '服务商' : '普通用户'}</span>
                        </div>
                    </div>
                    <form onsubmit="updateProfile(event)">
                        <div class="form-group"><label>用户名</label><input type="text" id="profileUsername" value="${escHtml(u.username)}" required></div>
                        <div class="form-group"><label>邮箱</label><input type="email" id="profileEmail" value="${escHtml(u.email)}" required></div>
                        <div class="form-group"><label>手机号</label><input type="tel" id="profilePhone" value="${escHtml(u.phone || '')}"></div>
                        <div class="form-group"><label>注册时间</label><input type="text" value="${formatDateTime(u.created_at)}" readonly></div>
                        <button type="submit" class="btn btn-primary">保存修改</button>
                    </form>
                </div>
                ${u.role !== 'provider' ? `
                <div class="profile-card" style="text-align:center;">
                    <h3 style="margin-bottom:16px;">成为服务商</h3>
                    <p style="color:var(--mid-gray);margin-bottom:20px;">发布您的服务，让更多人预约！</p>
                    <button class="btn btn-primary" style="background:var(--orange);" onclick="showModal('providerRegisterModal')">立即成为服务商</button>
                </div>` : ''}
            `;
        }
    });
}

function updateProfile(e) {
    e.preventDefault();
    const data = { username: $('profileUsername').value.trim(), email: $('profileEmail').value.trim(), phone: $('profilePhone').value.trim() };
    api('/api/auth/profile', { method: 'PUT', body: JSON.stringify(data) }).then(({ status, data: resp }) => {
        if (status === 200) { currentUser = resp.user; localStorage.setItem('user', JSON.stringify(resp.user)); updateNavState(); showToast('资料更新成功！', 'success'); }
        else { showToast(resp.error || '更新失败', 'error'); }
    });
}

// ==================== My Provider Application ====================
function loadMyApplication() {
    if (!currentUser) { showToast('请先登录', 'warning'); showModal('loginModal'); return; }
    const container = $('myApplicationContent');
    if (!container) return;
    container.innerHTML = '<div class="loading">加载中</div>';
    
    api('/api/providers/my-application').then(({ status, data }) => {
        if (data.applications && data.applications.length > 0) {
            const app = data.applications[0];
            const statusMap = { 'pending': '审核中', 'approved': '已通过', 'rejected': '未通过' };
            const statusClass = { 'pending': 'status-pending', 'approved': 'status-confirmed', 'rejected': 'status-cancelled' };
            const badgeMap = { 'pending': '⏳ 审核中', 'approved': '✅ 已通过', 'rejected': '❌ 未通过' };
            
            container.innerHTML = `
                <div class="profile-card">
                    <div class="profile-header" style="margin-bottom:20px;">
                        <div class="profile-avatar-lg">${escHtml(app.name.charAt(0))}</div>
                        <div>
                            <div class="profile-name">${escHtml(app.name)}</div>
                            <span class="profile-role">${escHtml(app.category)}</span>
                        </div>
                    </div>
                    
                    <div class="application-status-card">
                        <div class="audit-progress">
                            <div class="audit-step ${app.audit_status === 'pending' || app.audit_status === 'approved' ? 'active' : ''}">
                                <div class="step-dot">1</div>
                                <span>已提交</span>
                            </div>
                            <div class="step-line ${app.audit_status === 'pending' || app.audit_status === 'approved' ? 'active' : ''}"></div>
                            <div class="audit-step ${app.audit_status === 'pending' ? 'active' : ''} ${app.audit_status === 'approved' ? 'active' : ''}">
                                <div class="step-dot">2</div>
                                <span>审核中</span>
                            </div>
                            <div class="step-line ${app.audit_status === 'approved' || app.audit_status === 'rejected' ? 'active' : ''}"></div>
                            <div class="audit-step ${app.audit_status === 'approved' || app.audit_status === 'rejected' ? 'active' : ''}">
                                <div class="step-dot ${app.audit_status === 'rejected' ? 'rejected' : ''}">${app.audit_status === 'approved' ? '✓' : app.audit_status === 'rejected' ? '✗' : '3'}</div>
                                <span>${app.audit_status === 'approved' ? '已通过' : app.audit_status === 'rejected' ? '未通过' : '已审核'}</span>
                            </div>
                        </div>
                        <div class="audit-result" style="text-align:center;margin-top:20px;">
                            <span class="status-badge ${statusClass[app.audit_status] || 'status-pending'}">${badgeMap[app.audit_status] || '审核中'}</span>
                            ${app.audit_comment ? `<p style="margin-top:12px;color:var(--mid-gray);">审核意见：${escHtml(app.audit_comment)}</p>` : ''}
                            ${app.audit_status === 'approved' ? `<p style="margin-top:12px;color:var(--green);">恭喜！您现在可以管理服务和预约了。</p>` : ''}
                            ${app.audit_status === 'rejected' ? `<p style="margin-top:12px;color:var(--mid-gray);">您可以根据审核意见修改后重新提交申请。</p>` : ''}
                        </div>
                    </div>
                    
                    <div class="profile-card" style="margin-top:20px;">
                        <h4 style="margin-bottom:12px;">申请信息</h4>
                        <div class="info-row"><span>服务商名称</span><span>${escHtml(app.name)}</span></div>
                        <div class="info-row"><span>分类</span><span>${escHtml(app.category)}</span></div>
                        <div class="info-row"><span>简介</span><span>${escHtml(app.description) || '无'}</span></div>
                        <div class="info-row"><span>地址</span><span>${escHtml(app.address) || '无'}</span></div>
                        <div class="info-row"><span>电话</span><span>${escHtml(app.phone) || '无'}</span></div>
                        <div class="info-row"><span>提交时间</span><span>${formatDateTime(app.created_at)}</span></div>
                    </div>
                </div>
            `;
        } else {
            container.innerHTML = `
                <div class="empty-state" style="padding:80px 24px;">
                    <div class="empty-icon">📋</div>
                    <h3>尚未提交申请</h3>
                    <p style="margin-bottom:20px;">提交服务商申请，通过审核后即可发布服务</p>
                    <button class="btn btn-primary" style="background:var(--orange);" onclick="showModal('providerRegisterModal')">立即申请</button>
                </div>
            `;
        }
    });
}

// ==================== Provider Registration ====================
function handleProviderRegister(e) {
    e.preventDefault();
    const errorEl = $('providerRegisterError');
    errorEl.textContent = '';
    const data = { name: $('provName').value.trim(), category: $('provCategory').value, description: $('provDescription').value.trim(), address: $('provAddress').value.trim(), phone: $('provPhone').value.trim() };
    api('/api/providers', { method: 'POST', body: JSON.stringify(data) }).then(({ status, data: resp }) => {
        if (status === 200) {
            closeModal('providerRegisterModal');
            showToast('申请已提交，请等待管理员审核', 'success');
            navigate('myApplication');
        } else { errorEl.textContent = resp.error || '提交失败'; }
    });
}

// ==================== Provider Dashboard ====================
async function loadProviderDashboard() {
    if (!currentUser || currentUser.role !== 'provider') { showToast('请先注册为服务商', 'warning'); return; }
    const container = $('providerDashboardContent');
    try {
        const { data: profile } = await api('/api/auth/profile');
        if (!profile.provider) {
            container.innerHTML = `<div class="empty-state" style="padding:80px 24px;"><div class="empty-icon">🏠</div><h3>您还没有注册为服务商</h3><p style="margin-bottom:20px;">注册成为服务商，发布您的服务</p><button class="btn btn-primary" style="background:var(--orange);" onclick="showModal('providerRegisterModal')">成为服务商</button></div>`;
            return;
        }
        currentProvider = profile.provider;
        const p = profile.provider;
        const { data: apptArr } = await api('/api/appointments/my');
        const appointments = Array.isArray(apptArr) ? apptArr : [];
        const pending = appointments.filter(a => a.status === 'pending').length;
        const confirmed = appointments.filter(a => a.status === 'confirmed').length;
        const completed = appointments.filter(a => a.status === 'completed').length;
        const { data: svcData } = await api('/api/services');
        const allServices = Array.isArray(svcData) ? svcData : (svcData.services || []);
        const myServices = allServices.filter(s => s.provider_id === p.id);
        container.innerHTML = `
            <div class="provider-dashboard">
                <div class="dashboard-stats">
                    <div class="dashboard-stat"><div class="stat-value">${myServices.length}</div><div class="stat-label">我的服务</div></div>
                    <div class="dashboard-stat"><div class="stat-value">${pending}</div><div class="stat-label">待确认</div></div>
                    <div class="dashboard-stat"><div class="stat-value">${confirmed}</div><div class="stat-label">已确认</div></div>
                    <div class="dashboard-stat"><div class="stat-value">${completed}</div><div class="stat-label">已完成</div></div>
                </div>
                <div class="dashboard-tabs">
                    <button class="dashboard-tab active" onclick="switchDashboardTab('services', this)">我的服务</button>
                    <button class="dashboard-tab" onclick="switchDashboardTab('appointments', this)">预约管理</button>
                    <button class="dashboard-tab" onclick="switchDashboardTab('coupons', this)">优惠券管理</button>
                    <button class="dashboard-tab" onclick="switchDashboardTab('stats', this)">数据统计</button>
                    <button class="dashboard-tab" onclick="switchDashboardTab('info', this)">服务商信息</button>
                </div>
                <div id="dashboardTabContent">
                    <div id="tab-services">
                        <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:16px;">
                            <h3>我的服务</h3>
                            <button class="btn btn-primary btn-sm" onclick="openAddServiceModal()">发布新服务</button>
                        </div>
                        <div class="services-grid">
                            ${myServices.length > 0 ? myServices.map(s => `
                                <div class="service-card">
                                    <div class="service-card-image ${getCategoryClass(s.category)}">
                                        <span class="icon-text">${getCategoryIcon(s.category)}</span>
                                    </div>
                                    <div class="service-card-body">
                                        <h3>${escHtml(s.name)}</h3>
                                        <p class="service-desc">${escHtml(s.description).substring(0, 60)}</p>
                                        <div class="service-card-meta">
                                            <span class="service-price">${s.price > 0 ? '¥' + s.price : '免费'}</span>
                                            <span class="service-duration">${s.duration}分钟</span>
                                        </div>
                                        <div style="display:flex;gap:8px;margin-top:12px;">
                                            <button class="btn btn-outline btn-sm" style="color:var(--orange);border-color:var(--orange);" onclick="editService(${s.id}, '${escHtml(s.name)}', '${escHtml(s.category)}', '${escHtml(s.description)}', ${s.price}, ${s.duration})">编辑</button>
                                            <button class="btn btn-primary btn-sm" style="background:#EF4444;" onclick="deleteService(${s.id})">删除</button>
                                        </div>
                                    </div>
                                </div>
                            `).join('') : '<div class="empty-state"><p>还没有发布服务</p></div>'}
                        </div>
                    </div>
                    <div id="tab-appointments" style="display:none;">
                        ${appointments.length > 0 ? appointments.map(a => `
                            <div class="appointment-card">
                                <div class="appointment-header">
                                    <span class="appointment-title">${escHtml(a.service_name)} - ${escHtml(a.user_name)}</span>
                                    <span class="status-badge status-${a.status}">${getStatusText(a.status)}</span>
                                </div>
                                <div class="appointment-info"><span>📅 ${a.appointment_date} ${a.appointment_time}</span><span>💰 ¥${a.service_price}</span></div>
                                ${a.notes ? `<p style="color:var(--mid-gray);">备注: ${escHtml(a.notes)}</p>` : ''}
                                <div class="appointment-actions">
                                    ${a.status === 'pending' ? `<button class="btn btn-primary btn-sm" style="background:var(--blue);" onclick="confirmAppointment(${a.id})">确认</button>` : ''}
                                    ${a.status === 'confirmed' ? `<button class="btn btn-primary btn-sm" style="background:var(--green);" onclick="completeAppointment(${a.id})">完成</button>` : ''}
                                    ${a.status !== 'cancelled' && a.status !== 'completed' ? `<button class="btn btn-primary btn-sm" style="background:#EF4444;" onclick="cancelAppointment(${a.id})">取消</button>` : ''}
                                </div>
                            </div>
                        `).join('') : '<div class="empty-state"><p>暂无预约</p></div>'}
                    </div>
                    <div id="tab-coupons" style="display:none;">
                        <div id="couponsTabContent"></div>
                    </div>
                    <div id="tab-info" style="display:none;">
                        <div class="profile-card">
                            <form onsubmit="updateProviderInfo(event)">
                                <div class="form-group"><label>服务商名称</label><input type="text" id="dashProvName" value="${escHtml(p.name)}" required></div>
                                <div class="form-group"><label>分类</label><select id="dashProvCategory">${['医疗','美容','健身','教育','家政','法律'].map(c => `<option value="${c}" ${p.category===c?'selected':''}>${c}</option>`).join('')}</select></div>
                                <div class="form-group"><label>简介</label><textarea id="dashProvDesc" rows="3">${escHtml(p.description)}</textarea></div>
                                <div class="form-group"><label>地址</label><input type="text" id="dashProvAddr" value="${escHtml(p.address)}"></div>
                                <div class="form-group"><label>电话</label><input type="tel" id="dashProvPhone" value="${escHtml(p.phone)}"></div>
                                <div class="form-group"><label>营业时间</label>
                                    <div style="display:grid;grid-template-columns:80px 1fr;gap:8px 12px;align-items:center;">
                                        ${['周一','周二','周三','周四','周五','周六','周日'].map(d => {
                                            const hours = p.business_hours ? (JSON.parse(p.business_hours)[d] || '') : '';
                                            return `<span style="color:var(--mid-gray);font-size:0.9rem;">${d}</span><input type="text" class="biz-hour-input" data-day="${d}" value="${hours}" placeholder="例: 09:00-18:00" style="padding:6px 10px;border:1px solid var(--border);border-radius:6px;font-size:0.85rem;">`;
                                        }).join('')}
                                    </div>
                                </div>
                                <button type="submit" class="btn btn-primary">保存</button>
                            </form>
                        </div>
                    </div>
                    <div id="tab-stats" style="display:none;">
                        <div class="profile-card">
                            <h3 style="margin-bottom:16px;">数据统计</h3>
                            <div style="display:flex;gap:12px;margin-bottom:16px;">
                                <select id="dashStatsPeriod" style="flex:1;padding:8px 12px;border:1px solid var(--border);border-radius:6px;font-size:0.9rem;">
                                    <option value="day">按天</option>
                                    <option value="month" selected>按月</option>
                                    <option value="year">按年</option>
                                </select>
                                <button class="btn btn-primary btn-sm" onclick="loadProviderStatsTab()">查询</button>
                            </div>
                            <div id="dashStatsTable"><p style="color:var(--mid-gray);text-align:center;padding:20px;">点击查询查看统计数据</p></div>
                        </div>
                    </div>
                </div>
            </div>
        `;
    } catch (e) {
        container.innerHTML = `<div class="empty-state" style="padding:80px 24px;"><p style="color:#EF4444;">加载失败: ${escHtml(e.message)}</p><button class="btn btn-primary" style="margin-top:16px;" onclick="loadProviderDashboard()">重试</button></div>`;
    }
}

function switchDashboardTab(tab, btn) {
    document.querySelectorAll('.dashboard-tab').forEach(t => t.classList.remove('active'));
    btn.classList.add('active');
    document.getElementById('tab-services').style.display = tab === 'services' ? 'block' : 'none';
    document.getElementById('tab-appointments').style.display = tab === 'appointments' ? 'block' : 'none';
    document.getElementById('tab-coupons').style.display = tab === 'coupons' ? 'block' : 'none';
    document.getElementById('tab-info').style.display = tab === 'info' ? 'block' : 'none';
    document.getElementById('tab-stats').style.display = tab === 'stats' ? 'block' : 'none';
    if (tab === 'coupons') loadProviderCoupons();
    if (tab === 'stats') loadProviderStatsTab();
}

function updateProviderInfo(e) {
    e.preventDefault();
    const bizHours = {};
    document.querySelectorAll('.biz-hour-input').forEach(inp => {
        if (inp.value.trim()) bizHours[inp.dataset.day] = inp.value.trim();
    });
    const data = {
        name: $('dashProvName').value.trim(),
        category: $('dashProvCategory').value,
        description: $('dashProvDesc').value.trim(),
        address: $('dashProvAddr').value.trim(),
        phone: $('dashProvPhone').value.trim(),
        business_hours: Object.keys(bizHours).length > 0 ? JSON.stringify(bizHours) : ''
    };
    api('/api/providers/' + currentProvider.id, { method: 'PUT', body: JSON.stringify(data) }).then(({ status, data: resp }) => {
        if (status === 200) { currentProvider = resp.provider; localStorage.setItem('provider', JSON.stringify(resp.provider)); showToast('信息更新成功！', 'success'); }
        else { showToast(resp.error || '更新失败', 'error'); }
    });
}

function loadProviderStatsTab() {
    const period = $('dashStatsPeriod')?.value || 'month';
    const container = $('dashStatsTable');
    if (!container || !currentProvider) return;
    container.innerHTML = '<div class="loading">加载中</div>';
    api('/api/stats/provider-time?provider_id=' + currentProvider.id + '&period=' + period).then(({ data }) => {
        const items = data.data || [];
        if (items.length === 0) {
            container.innerHTML = '<p style="color:var(--mid-gray);text-align:center;padding:20px;">暂无数据</p>';
            return;
        }
        container.innerHTML = '<table style="width:100%;border-collapse:collapse;font-size:0.85rem;"><thead><tr style="background:var(--light-gray);"><th style="padding:10px 12px;text-align:left;border-bottom:1px solid var(--border);">时段</th><th style="padding:10px 12px;text-align:right;border-bottom:1px solid var(--border);">预约数</th><th style="padding:10px 12px;text-align:right;border-bottom:1px solid var(--border);">收入</th><th style="padding:10px 12px;text-align:right;border-bottom:1px solid var(--border);">平均评分</th><th style="padding:10px 12px;text-align:right;border-bottom:1px solid var(--border);">评价数</th></tr></thead><tbody>' +
            items.map(i => '<tr><td style="padding:10px 12px;border-bottom:1px solid var(--border);">' + escHtml(i.period) + '</td><td style="padding:10px 12px;text-align:right;border-bottom:1px solid var(--border);">' + i.appointment_count + '</td><td style="padding:10px 12px;text-align:right;border-bottom:1px solid var(--border);">¥' + (i.revenue || 0).toFixed(2) + '</td><td style="padding:10px 12px;text-align:right;border-bottom:1px solid var(--border);">' + (i.avg_rating || 0).toFixed(1) + '</td><td style="padding:10px 12px;text-align:right;border-bottom:1px solid var(--border);">' + i.review_count + '</td></tr>').join('') +
        '</tbody></table>';
    }).catch(() => {
        container.innerHTML = '<p style="color:var(--red);text-align:center;padding:20px;">加载失败</p>';
    });
}

// ==================== Admin Dashboard ====================
let currentAdminTab = 'applications';

function loadAdminDashboard() {
    if (!currentUser || currentUser.role !== 'admin') {
        showToast('无权限访问', 'error');
        navigate('dashboard');
        return;
    }
    switchAdminTab(currentAdminTab, document.querySelector('.dashboard-tab.active'));
}

function switchAdminTab(tab, btn) {
    currentAdminTab = tab;
    document.querySelectorAll('.dashboard-tab').forEach(t => t.classList.remove('active'));
    if (btn) btn.classList.add('active');
    
    const container = $('adminDashboardContent');
    if (!container) return;
    container.innerHTML = '<div class="loading">加载中</div>';
    
    if (tab === 'applications') {
        loadAdminApplications();
    } else if (tab === 'users') {
        loadAdminUsers();
    }
}

function loadAdminApplications() {
    const container = $('adminDashboardContent');
    if (!container) return;
    
    api('/api/providers/audit/all').then(({ data }) => {
        if (data && data.length > 0) {
            const statusMap = { 'pending': '待审核', 'approved': '已通过', 'rejected': '已拒绝' };
            const statusClass = { 'pending': 'status-pending', 'approved': 'status-confirmed', 'rejected': 'status-cancelled' };
            
            container.innerHTML = `
                <div style="max-width:900px;margin:0 auto;padding:0 24px;">
                    <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:20px;">
                        <h3>服务商入驻申请 (${data.length})</h3>
                    </div>
                    <div class="admin-applications-list">
                        ${data.map(p => `
                            <div class="appointment-card ${p.audit_status === 'pending' ? 'highlight' : ''}">
                                <div class="appointment-header">
                                    <span class="appointment-title">${escHtml(p.name)}</span>
                                    <span class="status-badge ${statusClass[p.audit_status] || 'status-pending'}">${statusMap[p.audit_status] || p.audit_status}</span>
                                </div>
                                <div class="appointment-info">
                                    <span>👤 申请人: ${escHtml(p.username)}</span>
                                    <span>📂 ${escHtml(p.category)}</span>
                                    <span>📅 ${formatDateTime(p.created_at)}</span>
                                </div>
                                <p style="color:var(--mid-gray);margin:8px 0;">${escHtml(p.description) || '暂无简介'}</p>
                                ${p.audit_comment ? `<p style="color:var(--orange);font-size:0.85rem;">审核意见: ${escHtml(p.audit_comment)}</p>` : ''}
                                <div class="appointment-actions" style="margin-top:12px;">
                                    <button class="btn btn-outline btn-sm" onclick="viewApplicationDetail(${p.id})">查看详情</button>
                                    ${p.audit_status === 'pending' ? `
                                        <button class="btn btn-primary btn-sm" style="background:var(--green);" onclick="approveApplication(${p.id})">通过</button>
                                        <button class="btn btn-primary btn-sm" style="background:#EF4444;" onclick="rejectApplication(${p.id})">拒绝</button>
                                    ` : ''}
                                </div>
                            </div>
                        `).join('')}
                    </div>
                </div>
            `;
        } else {
            container.innerHTML = '<div class="empty-state" style="padding:80px 24px;"><div class="empty-icon">📋</div><h3>暂无申请</h3><p>暂时没有服务商入驻申请</p></div>';
        }
    });
}

function loadAdminUsers() {
    const container = $('adminDashboardContent');
    if (!container) return;
    
    api('/api/users').then(({ data }) => {
        if (data.users && data.users.length > 0) {
            const roleMap = { 'user': '普通用户', 'provider': '服务商', 'admin': '管理员' };
            container.innerHTML = `
                <div style="max-width:900px;margin:0 auto;padding:0 24px;">
                    <h3 style="margin-bottom:20px;">用户管理 (${data.users.length})</h3>
                    <div class="admin-users-list">
                        ${data.users.map(u => `
                            <div class="appointment-card">
                                <div class="appointment-header">
                                    <span class="appointment-title">${escHtml(u.username)}</span>
                                    <span class="status-badge ${u.role === 'admin' ? 'status-confirmed' : u.role === 'provider' ? 'status-pending' : ''}">${roleMap[u.role] || u.role}</span>
                                </div>
                                <div class="appointment-info">
                                    <span>📧 ${escHtml(u.email)}</span>
                                    <span>📱 ${u.phone || '未填写'}</span>
                                    <span>📅 ${formatDateTime(u.created_at)}</span>
                                </div>
                            </div>
                        `).join('')}
                    </div>
                </div>
            `;
        } else {
            container.innerHTML = '<div class="empty-state" style="padding:80px 24px;"><div class="empty-icon">👤</div><h3>暂无用户</h3></div>';
        }
    });
}

function viewApplicationDetail(id) {
    api('/api/providers/' + id).then(({ data }) => {
        const statusMap = { 'pending': '待审核', 'approved': '已通过', 'rejected': '已拒绝' };
        const statusClass = { 'pending': 'status-pending', 'approved': 'status-confirmed', 'rejected': 'status-cancelled' };
        $('auditModalBody').innerHTML = `
            <div class="profile-card">
                <div class="profile-header">
                    <div class="profile-avatar-lg">${escHtml(data.name.charAt(0))}</div>
                    <div>
                        <div class="profile-name">${escHtml(data.name)}</div>
                        <span class="profile-role">${escHtml(data.category)}</span>
                    </div>
                </div>
                <div class="info-row"><span>状态</span><span class="status-badge ${statusClass[data.audit_status]}">${statusMap[data.audit_status]}</span></div>
                <div class="info-row"><span>简介</span><span>${escHtml(data.description) || '无'}</span></div>
                <div class="info-row"><span>地址</span><span>${escHtml(data.address) || '无'}</span></div>
                <div class="info-row"><span>电话</span><span>${escHtml(data.phone) || '无'}</span></div>
                <div class="info-row"><span>许可证号</span><span>${escHtml(data.license_number) || '无'}</span></div>
                <div class="info-row"><span>提交时间</span><span>${formatDateTime(data.created_at)}</span></div>
                ${data.audit_comment ? `<div class="info-row"><span>审核意见</span><span style="color:var(--orange);">${escHtml(data.audit_comment)}</span></div>` : ''}
            </div>
            ${data.audit_status === 'pending' ? `
                <div style="display:flex;gap:12px;margin-top:20px;">
                    <button class="btn btn-primary" style="flex:1;background:var(--green);" onclick="approveApplication(${data.id})">✅ 通过</button>
                    <button class="btn btn-primary" style="flex:1;background:#EF4444;" onclick="closeModal('auditModal');rejectApplication(${data.id})">❌ 拒绝</button>
                </div>
            ` : ''}
        `;
        showModal('auditModal');
    });
}

function approveApplication(id) {
    const comment = prompt('请输入审核意见（可选）：');
    api('/api/providers/' + id + '/audit', { method: 'POST', body: JSON.stringify({ audit_status: 'approved', audit_comment: comment || '' }) }).then(({ status, data: resp }) => {
        if (status === 200) {
            showToast('已通过该申请', 'success');
            closeModal('auditModal');
            loadAdminApplications();
        } else {
            showToast(resp.error || '操作失败', 'error');
        }
    });
}

function rejectApplication(id) {
    const comment = prompt('请输入拒绝原因：');
    if (!comment) { showToast('请填写拒绝原因', 'warning'); return; }
    api('/api/providers/' + id + '/audit', { method: 'POST', body: JSON.stringify({ audit_status: 'rejected', audit_comment: comment }) }).then(({ status, data: resp }) => {
        if (status === 200) {
            showToast('已拒绝该申请', 'info');
            closeModal('auditModal');
            loadAdminApplications();
        } else {
            showToast(resp.error || '操作失败', 'error');
        }
    });
}

// ==================== Service CRUD (Provider) ====================
function openAddServiceModal(editData) {
    if (editData) {
        $('addServiceTitle').textContent = '编辑服务';
        $('editServiceId').value = editData.id;
        $('svcName').value = editData.name;
        $('svcCategory').value = editData.category;
        $('svcDescription').value = editData.description;
        $('svcPrice').value = editData.price;
        $('svcDuration').value = editData.duration;
    } else {
        $('addServiceTitle').textContent = '发布服务';
        $('editServiceId').value = '';
        $('svcName').value = ''; $('svcCategory').value = ''; $('svcDescription').value = '';
        $('svcPrice').value = '0'; $('svcDuration').value = '60';
    }
    $('addServiceError').textContent = '';
    showModal('addServiceModal');
}

function editService(id, name, category, description, price, duration) {
    openAddServiceModal({ id, name, category, description, price, duration });
}

function handleAddService(e) {
    e.preventDefault();
    const errorEl = $('addServiceError');
    errorEl.textContent = '';
    const data = { name: $('svcName').value.trim(), category: $('svcCategory').value, description: $('svcDescription').value.trim(), price: parseFloat($('svcPrice').value) || 0, duration: parseInt($('svcDuration').value) || 60 };
    const editId = $('editServiceId').value;
    const url = editId ? '/api/services/' + editId : '/api/services';
    const method = editId ? 'PUT' : 'POST';
    api(url, { method, body: JSON.stringify(data) }).then(({ status, data: resp }) => {
        if (status === 200) { closeModal('addServiceModal'); showToast(editId ? '服务更新成功！' : '服务发布成功！', 'success'); loadProviderDashboard(); }
        else { errorEl.textContent = resp.error || '操作失败'; }
    });
}

function deleteService(id) {
    if (!confirm('确定要删除这个服务吗？')) return;
    api('/api/services/' + id, { method: 'DELETE' }).then(({ status, data }) => {
        if (status === 200) { showToast('服务已删除', 'success'); loadProviderDashboard(); }
        else { showToast(data.error || '删除失败', 'error'); }
    });
}

// ==================== Coupons ====================
function toggleCouponAmount() {
    const type = $('couponType').value;
    const label = $('couponAmountLabel');
    const input = $('couponAmount');
    if (type === 'fixed') {
        label.textContent = '优惠金额 (元)';
        input.step = '0.01';
    } else {
        label.textContent = '折扣百分比 (%)';
        input.step = '1';
        input.max = '100';
    }
}

function loadProviderCoupons() {
    const container = document.getElementById('couponsTabContent');
    if (!container) return;
    container.innerHTML = '<div class="loading">加载中</div>';
    
    api('/api/coupons/provider').then(({ data }) => {
        const coupons = Array.isArray(data) ? data : (data.coupons || []);
        if (coupons.length > 0) {
            container.innerHTML = `
                <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:16px;">
                    <h3>我的优惠券</h3>
                    <button class="btn btn-primary btn-sm" onclick="openAddCouponModal()">创建优惠券</button>
                </div>
                <div class="coupons-grid">
                    ${coupons.map(c => `
                        <div class="coupon-card">
                            <div class="coupon-left">
                                <span class="coupon-amount">${c.coupon_type === 'fixed' ? '¥' + c.discount_amount : c.discount_percent + '%'}</span>
                                <span class="coupon-condition">满${c.min_amount}可用</span>
                            </div>
                            <div class="coupon-right">
                                <h4>${escHtml(c.name)}</h4>
                                <p>${escHtml(c.description)}</p>
                                <div class="coupon-info">
                                    <span>总量: ${c.total_count}</span>
                                    <span>已用: ${c.used_count}</span>
                                    <span>状态: ${c.status === 'active' ? '生效中' : '已过期'}</span>
                                </div>
                                <div class="coupon-actions">
                                    <button class="btn btn-outline btn-sm" onclick="editCoupon(${c.id})">编辑</button>
                                    <button class="btn btn-primary btn-sm" style="background:#EF4444;" onclick="deleteCoupon(${c.id})">删除</button>
                                </div>
                            </div>
                        </div>
                    `).join('')}
                </div>
            `;
        } else {
            container.innerHTML = `<div class="empty-state"><p>还没有创建优惠券</p><button class="btn btn-primary btn-sm" onclick="openAddCouponModal()">创建优惠券</button></div>`;
        }
    });
}

function openAddCouponModal(couponData) {
    const modal = document.getElementById('addCouponModal');
    if (!modal) return;
    
    if (couponData) {
        $('couponTitle').textContent = '编辑优惠券';
        $('editCouponId').value = couponData.id;
        $('couponName').value = couponData.name;
        $('couponDesc').value = couponData.description;
        $('couponType').value = couponData.coupon_type;
        $('couponAmount').value = couponData.coupon_type === 'fixed' ? couponData.discount_amount : couponData.discount_percent;
        $('couponMinAmount').value = couponData.min_amount;
        $('couponTotalCount').value = couponData.total_count;
        $('couponStartTime').value = couponData.start_time ? couponData.start_time.split(' ')[0] : '';
        $('couponEndTime').value = couponData.end_time ? couponData.end_time.split(' ')[0] : '';
    } else {
        $('couponTitle').textContent = '创建优惠券';
        $('editCouponId').value = '';
        $('couponName').value = '';
        $('couponDesc').value = '';
        $('couponType').value = 'fixed';
        $('couponAmount').value = '';
        $('couponMinAmount').value = '';
        $('couponTotalCount').value = '100';
        const today = new Date().toISOString().split('T')[0];
        $('couponStartTime').value = today;
        const nextMonth = new Date();
        nextMonth.setMonth(nextMonth.getMonth() + 1);
        $('couponEndTime').value = nextMonth.toISOString().split('T')[0];
    }
    $('addCouponError').textContent = '';
    showModal('addCouponModal');
}

function editCoupon(id) {
    api('/api/coupons/' + id).then(({ data }) => {
        if (data.coupon) {
            openAddCouponModal(data.coupon);
        }
    });
}

function handleAddCoupon(e) {
    e.preventDefault();
    const errorEl = $('addCouponError');
    errorEl.textContent = '';
    
    const data = {
        name: $('couponName').value.trim(),
        description: $('couponDesc').value.trim(),
        coupon_type: $('couponType').value,
        discount_amount: $('couponType').value === 'fixed' ? parseFloat($('couponAmount').value) || 0 : 0,
        discount_percent: $('couponType').value === 'percent' ? parseInt($('couponAmount').value) || 0 : 0,
        min_amount: parseFloat($('couponMinAmount').value) || 0,
        total_count: parseInt($('couponTotalCount').value) || 100,
        start_time: $('couponStartTime').value,
        end_time: $('couponEndTime').value
    };
    
    const editId = $('editCouponId').value;
    const url = editId ? '/api/coupons/' + editId : '/api/coupons';
    const method = editId ? 'PUT' : 'POST';
    
    api(url, { method, body: JSON.stringify(data) }).then(({ status, data: resp }) => {
        if (status === 200) {
            closeModal('addCouponModal');
            showToast(editId ? '优惠券更新成功！' : '优惠券创建成功！', 'success');
            loadProviderCoupons();
        } else {
            errorEl.textContent = resp.error || '操作失败';
        }
    });
}

function deleteCoupon(id) {
    if (!confirm('确定要删除这个优惠券吗？')) return;
    api('/api/coupons/' + id, { method: 'DELETE' }).then(({ status, data }) => {
        if (status === 200) { showToast('优惠券已删除', 'success'); loadProviderCoupons(); }
        else { showToast(data.error || '删除失败', 'error'); }
    });
}

function loadUserCoupons() {
    const container = document.getElementById('userCouponsContent');
    if (!container) return;
    container.innerHTML = '<div class="loading">加载中</div>';
    
    api('/api/coupons/user').then(({ data }) => {
        if (data.user_coupons && data.user_coupons.length > 0) {
            container.innerHTML = data.user_coupons.map(uc => `
                <div class="coupon-card ${uc.status === 'used' ? 'used' : ''}">
                    <div class="coupon-left">
                        <span class="coupon-amount">${uc.coupon_type === 'fixed' ? '¥' + uc.discount_amount : uc.discount_percent + '%'}</span>
                        <span class="coupon-condition">满${uc.min_amount}可用</span>
                    </div>
                    <div class="coupon-right">
                        <h4>${escHtml(uc.coupon_name)}</h4>
                        <p>${escHtml(uc.provider_name || '')}</p>
                        <div class="coupon-info">
                            <span>有效期: ${formatDate(uc.start_time)} - ${formatDate(uc.end_time)}</span>
                            <span>状态: ${uc.status === 'unused' ? '未使用' : uc.status === 'used' ? '已使用' : '已过期'}</span>
                        </div>
                        ${uc.status === 'unused' ? `<button class="btn btn-primary btn-sm" onclick="useUserCoupon(${uc.id})">去使用</button>` : ''}
                    </div>
                </div>
            `).join('');
        } else {
            container.innerHTML = '<div class="empty-state"><div class="empty-icon">🎫</div><h3>暂无优惠券</h3><p>快去领取优惠券吧！</p></div>';
        }
    });
}

function loadReports() {
    const container = document.getElementById('reportsContent');
    if (!container) return;
    container.innerHTML = '<div class="loading">加载中</div>';

    Promise.all([
        api('/api/stats'),
        api('/api/stats/daily'),
        api('/api/stats/categories'),
        api('/api/stats/providers'),
        api('/api/stats/appointments'),
        api('/api/stats/coupons')
    ]).then(([statsRes, dailyRes, categoriesRes, providersRes, appointmentsRes, couponsRes]) => {
        renderStatsOverview(statsRes.data, couponsRes.data);
        renderTrendChart(dailyRes.data);
        renderCategoryList(categoriesRes.data);
        renderProviderRanking(providersRes.data);
        renderStatusDistribution(appointmentsRes.data);
        renderCouponStats(couponsRes.data);
        const sel = $('providerStatsSelect');
        if (sel && providersRes.data) {
            sel.innerHTML = '<option value="">选择服务商</option>' + providersRes.data.map(p => '<option value="' + p.provider_id + '">' + escHtml(p.provider_name) + '</option>').join('');
        }
        container.style.opacity = '1';
    }).catch(err => {
        container.innerHTML = '<div class="empty-state"><div class="empty-icon">📊</div><h3>加载失败</h3><p>请稍后重试</p></div>';
    });
}

function renderStatsOverview(stats, couponStats) {
    $('reportTotalUsers').textContent = stats.total_users || 0;
    $('reportTotalProviders').textContent = stats.total_providers || 0;
    $('reportTotalServices').textContent = stats.total_services || 0;
    $('reportTotalAppointments').textContent = stats.total_appointments || 0;
    $('reportTotalRevenue').textContent = '¥' + (stats.total_revenue || 0).toFixed(2);
    $('reportTotalCoupons').textContent = couponStats.total_coupons || 0;
}

function renderTrendChart(dailyData) {
    const barsContainer = $('trendBars');
    const labelsContainer = $('trendLabels');
    if (!barsContainer || !labelsContainer) return;

    const maxAppointments = Math.max(...dailyData.map(d => d.new_appointments || 0), 1);
    
    barsContainer.innerHTML = dailyData.map(d => {
        const height = ((d.new_appointments || 0) / maxAppointments) * 100;
        return `<div style="display:flex;flex-direction:column;align-items:center;height:100%;justify-content:flex-end;"><div class="chart-bar" style="height:${Math.max(height, 5)}%;"></div></div>`;
    }).join('');

    labelsContainer.innerHTML = dailyData.map(d => {
        const date = d.date ? d.date.slice(5) : '';
        return `<div class="chart-label">${date}</div>`;
    }).join('');
}

function renderCategoryList(categories) {
    const container = $('categoryList');
    if (!container) return;

    if (!categories || categories.length === 0) {
        container.innerHTML = '<div class="empty-state" style="padding:20px;"><p>暂无分类数据</p></div>';
        return;
    }

    const colors = ['var(--blue)', 'var(--green)', 'var(--orange)', 'var(--purple)', 'var(--pink)', 'var(--red)'];
    
    container.innerHTML = categories.map((c, i) => `
        <div class="category-item">
            <div class="category-color" style="background:${colors[i % colors.length]};"></div>
            <div class="category-info">
                <h4>${escHtml(c.category)}</h4>
                <p>${c.service_count}个服务 · ${c.appointment_count}次预约</p>
            </div>
            <div class="category-revenue">¥${(c.revenue || 0).toFixed(0)}</div>
        </div>
    `).join('');
}

function renderProviderRanking(providers) {
    const container = $('providerRanking');
    if (!container) return;

    if (!providers || providers.length === 0) {
        container.innerHTML = '<div class="empty-state" style="padding:20px;"><p>暂无服务商数据</p></div>';
        return;
    }

    container.innerHTML = providers.map((p, i) => `
        <div class="provider-rank-item">
            <div class="rank-badge ${i < 3 ? 'top3' : ''}">${i + 1}</div>
            <div class="provider-rank-info">
                <h4>${escHtml(p.provider_name)}</h4>
                <p>${p.service_count}个服务 · ${p.appointment_count}次预约</p>
            </div>
            <div class="provider-rank-revenue">¥${(p.revenue || 0).toFixed(0)}</div>
        </div>
    `).join('');
}

function renderStatusDistribution(statusData) {
    const container = $('statusDistribution');
    if (!container) return;

    if (!statusData || statusData.length === 0) {
        container.innerHTML = '<div class="empty-state" style="padding:20px;"><p>暂无预约数据</p></div>';
        return;
    }

    const total = statusData.reduce((sum, s) => sum + (s.count || 0), 0);
    const statusLabels = { 'pending': '待确认', 'confirmed': '已确认', 'completed': '已完成', 'cancelled': '已取消' };

    container.innerHTML = statusData.map(s => `
        <div class="status-item">
            <span class="status-label">${statusLabels[s.status] || s.status}</span>
            <div class="status-bar-bg">
                <div class="status-bar-fill ${s.status}" style="width:${total > 0 ? (s.count / total * 100) : 0}%;"></div>
            </div>
            <span class="status-count">${s.count}</span>
        </div>
    `).join('');
}

function renderCouponStats(couponStats) {
    const container = $('couponStats');
    if (!container) return;

    container.innerHTML = `
        <div class="coupon-stat-item">
            <div class="stat-value">${couponStats.total_coupons || 0}</div>
            <div class="stat-label">优惠券总数</div>
        </div>
        <div class="coupon-stat-item">
            <div class="stat-value">${couponStats.total_issued || 0}</div>
            <div class="stat-label">已发放</div>
        </div>
        <div class="coupon-stat-item">
            <div class="stat-value">${couponStats.total_used || 0}</div>
            <div class="stat-label">已使用</div>
        </div>
        <div class="coupon-stat-item">
            <div class="stat-value">¥${(couponStats.total_discount || 0).toFixed(0)}</div>
            <div class="stat-label">优惠金额</div>
        </div>
    `;
}

function loadProviderTimeStats() {
    const sel = $('providerStatsSelect');
    const period = $('providerStatsPeriod')?.value || 'month';
    const providerId = sel?.value;
    if (!providerId) { showToast('请选择服务商', 'warning'); return; }
    const container = $('providerTimeStatsTable');
    if (!container) return;
    container.innerHTML = '<div class="loading">加载中</div>';
    api('/api/stats/provider-time?provider_id=' + providerId + '&period=' + period).then(({ data }) => {
        const items = data.data || [];
        if (items.length === 0) {
            container.innerHTML = '<p style="color:var(--mid-gray);text-align:center;padding:20px;">暂无数据</p>';
            return;
        }
        container.innerHTML = '<table style="width:100%;border-collapse:collapse;font-size:0.85rem;"><thead><tr style="background:var(--light-gray);"><th style="padding:10px 12px;text-align:left;border-bottom:1px solid var(--border);">时段</th><th style="padding:10px 12px;text-align:right;border-bottom:1px solid var(--border);">预约数</th><th style="padding:10px 12px;text-align:right;border-bottom:1px solid var(--border);">收入</th><th style="padding:10px 12px;text-align:right;border-bottom:1px solid var(--border);">平均评分</th><th style="padding:10px 12px;text-align:right;border-bottom:1px solid var(--border);">评价数</th></tr></thead><tbody>' +
            items.map(i => '<tr><td style="padding:10px 12px;border-bottom:1px solid var(--border);">' + escHtml(i.period) + '</td><td style="padding:10px 12px;text-align:right;border-bottom:1px solid var(--border);">' + i.appointment_count + '</td><td style="padding:10px 12px;text-align:right;border-bottom:1px solid var(--border);">¥' + (i.revenue || 0).toFixed(2) + '</td><td style="padding:10px 12px;text-align:right;border-bottom:1px solid var(--border);">' + (i.avg_rating || 0).toFixed(1) + '</td><td style="padding:10px 12px;text-align:right;border-bottom:1px solid var(--border);">' + i.review_count + '</td></tr>').join('') +
        '</tbody></table>';
    }).catch(() => {
        container.innerHTML = '<p style="color:var(--red);text-align:center;padding:20px;">加载失败</p>';
    });
}

function claimCoupon(couponId) {
    api('/api/coupons/' + couponId + '/claim', { method: 'POST' }).then(({ status, data }) => {
        if (status === 200) {
            showToast('领取成功！', 'success');
            loadUserCoupons();
        } else {
            showToast(data.error || '领取失败', 'error');
        }
    });
}

function useUserCoupon(userCouponId) {
    api('/api/coupons/user/' + userCouponId + '/use', { method: 'POST' }).then(({ status, data }) => {
        if (status === 200) {
            showToast('优惠券使用成功！', 'success');
            loadUserCoupons();
        } else {
            showToast(data.error || '使用失败', 'error');
        }
    });
}

// ==================== Utilities ====================
function getCategoryIcon(category) {
    const icons = { '医疗': '⚤', '美容': '♠', '健身': '♢', '教育': '♣', '家政': '♥', '法律': '♦' };
    return icons[category] || '♦';
}

function getCategoryClass(category) {
    const map = { '医疗': 'medical', '美容': 'beauty', '健身': 'fitness', '教育': 'education', '家政': 'household', '法律': 'legal' };
    return map[category] || 'medical';
}

function getStatusText(status) {
    const map = { 'pending': '待确认', 'confirmed': '已确认', 'completed': '已完成', 'cancelled': '已取消' };
    return map[status] || status;
}

function escHtml(str) {
    if (!str) return '';
    const div = document.createElement('div');
    div.textContent = str;
    return div.innerHTML;
}

// ==================== Init ====================
document.addEventListener('DOMContentLoaded', () => {
    const loggedIn = initAuth();
    if (loggedIn) {
        navigateToDefaultPage();
    } else {
        currentPage = 'home';
        loadHomePage();
    }
    initScrollReveal();
    animateCounters();

    // Combined scroll handler for performance
    let ticking = false;
    window.addEventListener('scroll', () => {
        if (!ticking) {
            requestAnimationFrame(() => {
                handleNavScroll();
                updateActiveNavLink();
                updateScrollProgress();
                updateBackToTop();
                handleParallax();
                ticking = false;
            });
            ticking = true;
        }
    }, { passive: true });

    // Initial nav check
    handleNavScroll();
    updateScrollProgress();
    updateBackToTop();
});
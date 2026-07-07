// ==================== API Configuration ====================
const API_BASE = '';

// ==================== State ====================
let currentUser = null;
let currentToken = null;
let currentProvider = null;
let currentPage = 'home';
let navigationHistory = [];

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

    // Show/hide landing page
    const homePage = document.getElementById('home');
    if (homePage) homePage.classList.toggle('active', page === 'home');

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
        case 'providerDashboard': loadProviderDashboard(); break;
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
        const homePage = document.getElementById('home');
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
            if (resp.user.role === 'provider') {
                showModal('providerRegisterModal');
                navigate('providerDashboard');
            } else {
                navigate('dashboard');
            }
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
    
    if (currentUser) {
        navActions.style.display = 'none';
        navUser.style.display = 'flex';
        userNameDisplay.textContent = currentUser.username;
        userAvatar.textContent = currentUser.username.charAt(0).toUpperCase();
        if (currentUser.role === 'provider') providerLink.style.display = 'block';
        loadUnreadCount();
    } else {
        navActions.style.display = 'flex';
        navUser.style.display = 'none';
        providerLink.style.display = 'none';
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

    // Set user name
    const nameEl = document.getElementById('dashUserName');
    if (nameEl) nameEl.textContent = currentUser.username;

    // Reset search
    const searchInput = document.getElementById('dashSearch');
    if (searchInput) searchInput.value = '';

    // Load data
    loadPromoServices();
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

function loadDashHotServices() {
    const container = document.getElementById('dashHotServices');
    if (!container) return;
    container.innerHTML = '<div class="loading">加载中</div>';

    const params = new URLSearchParams();
    if (dashCategory) params.append('category', dashCategory);

    api('/api/services?' + params.toString()).then(({ data }) => {
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
    loadDashHotServices();
}

// ==================== Services ====================
function loadServices() {
    const keyword = $('serviceSearch')?.value || '';
    const category = $('serviceCategoryFilter')?.value || '';
    const params = new URLSearchParams();
    if (keyword) params.append('keyword', keyword);
    if (category) params.append('category', category);
    
    const container = document.getElementById('allServices');
    if (!container) return;
    container.innerHTML = '<div class="loading">加载中</div>';
    
    api('/api/services?' + params.toString()).then(({ data }) => {
        if (data.services && data.services.length > 0) {
            container.innerHTML = data.services.map(s => `
                <div class="service-card" onclick="navigate('serviceDetail', ${s.id})">
                    <div class="service-card-image ${getCategoryClass(s.category)}">
                        <span class="icon-text">${getCategoryIcon(s.category)}</span>
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
    
    api('/api/services/' + serviceId).then(({ data }) => {
        if (data.service) {
            const s = data.service;
            const p = data.provider;
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
                            <div><span class="label">评分</span><span class="value" style="color:var(--orange);">★ ${(data.avg_rating || 0).toFixed(1)}</span></div>
                        </div>
                        <button class="btn btn-primary" style="margin-top:20px;background:var(--orange);" onclick="openAppointmentModal(${s.id}, '${escHtml(s.name)}')">立即预约</button>
                    </div>
                    <div class="detail-section">
                        <h2>服务商信息</h2>
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
                        <h2>用户评价 (${data.reviews ? data.reviews.length : 0})</h2>
                        ${data.reviews && data.reviews.length > 0 ? data.reviews.map(r => `
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
    const params = new URLSearchParams();
    if (category) params.append('category', category);
    
    const container = document.getElementById('allProviders');
    if (!container) return;
    container.innerHTML = '<div class="loading">加载中</div>';
    
    api('/api/providers?' + params.toString()).then(({ data }) => {
        if (data.providers && data.providers.length > 0) {
            container.innerHTML = data.providers.map(p => `
                <div class="service-card" onclick="navigate('services');$('serviceCategoryFilter').value='${p.category}';loadServices();">
                    <div class="service-card-image ${getCategoryClass(p.category)}">
                        <span class="icon-text">${p.name.charAt(0)}</span>
                    </div>
                    <div class="service-card-body">
                        <h3>${escHtml(p.name)}</h3>
                        <span class="profile-role">${escHtml(p.category)}</span>
                        <p class="service-desc" style="margin-top:8px;">${escHtml(p.description) || '暂无简介'}</p>
                        <p style="color:var(--mid-gray);margin-top:8px;">${escHtml(p.address)}</p>
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
    
    const timeSelect = $('apptTime');
    timeSelect.innerHTML = '<option value="">请选择时间</option>';
    for (let h = 8; h <= 20; h++) {
        for (let m = 0; m < 60; m += 30) {
            const time = `${String(h).padStart(2,'0')}:${String(m).padStart(2,'0')}`;
            timeSelect.innerHTML += `<option value="${time}">${time}</option>`;
        }
    }
    showModal('appointmentModal');
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
        else { errorEl.textContent = resp.error || '预约失败'; }
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

// ==================== Provider Registration ====================
function handleProviderRegister(e) {
    e.preventDefault();
    const errorEl = $('providerRegisterError');
    errorEl.textContent = '';
    const data = { name: $('provName').value.trim(), category: $('provCategory').value, description: $('provDescription').value.trim(), address: $('provAddress').value.trim(), phone: $('provPhone').value.trim() };
    api('/api/providers', { method: 'POST', body: JSON.stringify(data) }).then(({ status, data: resp }) => {
        if (status === 200) {
            currentProvider = resp.provider; currentUser.role = 'provider';
            localStorage.setItem('provider', JSON.stringify(resp.provider));
            localStorage.setItem('user', JSON.stringify(currentUser));
            closeModal('providerRegisterModal'); updateNavState();
            showToast('服务商注册成功！', 'success'); navigate('providerDashboard');
        } else { errorEl.textContent = resp.error || '注册失败'; }
    });
}

// ==================== Provider Dashboard ====================
function loadProviderDashboard() {
    if (!currentUser || currentUser.role !== 'provider') { showToast('请先注册为服务商', 'warning'); return; }
    const container = $('providerDashboardContent');
    api('/api/auth/profile').then(({ data }) => {
        if (data.provider) {
            currentProvider = data.provider;
            const p = data.provider;
            api('/api/appointments').then(({ data: apptData }) => {
                const appointments = apptData.appointments || [];
                const pending = appointments.filter(a => a.status === 'pending').length;
                const confirmed = appointments.filter(a => a.status === 'confirmed').length;
                const completed = appointments.filter(a => a.status === 'completed').length;
                api('/api/services').then(({ data: svcData }) => {
                    const myServices = (svcData.services || []).filter(s => s.provider_id === p.id);
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
                                <div id="tab-info" style="display:none;">
                                    <div class="profile-card">
                                        <form onsubmit="updateProviderInfo(event)">
                                            <div class="form-group"><label>服务商名称</label><input type="text" id="dashProvName" value="${escHtml(p.name)}" required></div>
                                            <div class="form-group"><label>分类</label><select id="dashProvCategory">${['医疗','美容','健身','教育','家政','法律'].map(c => `<option value="${c}" ${p.category===c?'selected':''}>${c}</option>`).join('')}</select></div>
                                            <div class="form-group"><label>简介</label><textarea id="dashProvDesc" rows="3">${escHtml(p.description)}</textarea></div>
                                            <div class="form-group"><label>地址</label><input type="text" id="dashProvAddr" value="${escHtml(p.address)}"></div>
                                            <div class="form-group"><label>电话</label><input type="tel" id="dashProvPhone" value="${escHtml(p.phone)}"></div>
                                            <button type="submit" class="btn btn-primary">保存</button>
                                        </form>
                                    </div>
                                </div>
                            </div>
                        </div>
                    `;
                });
            });
        } else {
            container.innerHTML = `<div class="empty-state" style="padding:80px 24px;"><div class="empty-icon">🏠</div><h3>您还没有注册为服务商</h3><p style="margin-bottom:20px;">注册成为服务商，发布您的服务</p><button class="btn btn-primary" style="background:var(--orange);" onclick="showModal('providerRegisterModal')">成为服务商</button></div>`;
        }
    });
}

function switchDashboardTab(tab, btn) {
    document.querySelectorAll('.dashboard-tab').forEach(t => t.classList.remove('active'));
    btn.classList.add('active');
    document.getElementById('tab-services').style.display = tab === 'services' ? 'block' : 'none';
    document.getElementById('tab-appointments').style.display = tab === 'appointments' ? 'block' : 'none';
    document.getElementById('tab-info').style.display = tab === 'info' ? 'block' : 'none';
}

function updateProviderInfo(e) {
    e.preventDefault();
    const data = { name: $('dashProvName').value.trim(), category: $('dashProvCategory').value, description: $('dashProvDesc').value.trim(), address: $('dashProvAddr').value.trim(), phone: $('dashProvPhone').value.trim() };
    api('/api/providers', { method: 'PUT', body: JSON.stringify(data) }).then(({ status, data: resp }) => {
        if (status === 200) { currentProvider = resp.provider; localStorage.setItem('provider', JSON.stringify(resp.provider)); showToast('信息更新成功！', 'success'); }
        else { showToast(resp.error || '更新失败', 'error'); }
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
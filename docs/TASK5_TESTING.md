# Task 5 Testing Guide - Authentication System

## 📋 Overview

Task 5 implements a complete authentication system with:
- ✅ Beautiful login page
- ✅ Session management (cookies)
- ✅ Protected admin routes
- ✅ Password change functionality
- ✅ Logout

---

## 🔐 Default Credentials

```
Username: admin
Password: admin123
```

**⚠️ IMPORTANT:** Change this password after first login!

---

## 🧪 Testing Steps

### 1️⃣ Upload Code

1. Open `esp32_wifi_feeder_v2.ino` in Arduino IDE
2. **Verify** (Ctrl+R) - check for compilation errors
3. **Upload** (Ctrl+U)
4. **Open Serial Monitor** (115200 baud)

Expected output:
```
✓ WiFi connected successfully!
  IP Address: 172.20.10.2
Authentication routes registered
HTTP server started
Ready at: http://172.20.10.2
```

---

### 2️⃣ Test Login Flow

#### A. Access Login Page

Open browser: **http://172.20.10.2/login**

Expected:
- 🎨 Beautiful gradient background (blue → green)
- 🐔 Chicken logo
- Login form with username & password fields
- "Đăng nhập" button

#### B. Try Wrong Credentials

Enter:
- Username: `admin`
- Password: `wrongpassword`
- Click "Đăng nhập"

Expected:
- ❌ Red error message: "Tên đăng nhập hoặc mật khẩu không đúng"
- Form shakes animation
- Button re-enabled

#### C. Login Successfully

Enter:
- Username: `admin`
- Password: `admin123`
- Click "Đăng nhập"

Expected:
- ✅ Redirect to `/admin`
- See admin dashboard
- "Logged in successfully!" message
- Logout button visible

Serial Monitor should show:
```
--- Login Request ---
Body: {"username":"admin","password":"admin123"}
New session created: abc123xyz...
Login successful: admin
```

---

### 3️⃣ Test Protected Routes

#### A. Try Access Without Login

1. Open **new incognito/private window**
2. Go to: **http://172.20.10.2/admin**

Expected:
- ↪️ Automatically redirected to `/login`
- See login page (not admin page)

Serial Monitor:
```
Access denied - not authenticated
```

#### B. Access After Login

1. Login normally (username: admin, password: admin123)
2. Go to: **http://172.20.10.2/admin**

Expected:
- ✅ Admin dashboard loads successfully
- No redirect

---

### 4️⃣ Test Session Management

#### A. Test Session Cookie

After logging in, open **DevTools** (F12):
1. Go to **Application** → **Cookies** → `http://172.20.10.2`
2. Look for cookie named `session`

Expected:
- Cookie exists
- Value: 32-character random string (e.g., `aBc123XyZ...`)
- Path: `/`
- HttpOnly: ✅ (for security)
- SameSite: `Strict`
- Max-Age: `3600` (1 hour)

#### B. Test Session Timeout

**QUICK TEST** (change SESSION_TIMEOUT temporarily):

1. Edit `config.h`:
```cpp
#define SESSION_TIMEOUT 30000  // 30 seconds (for testing)
```

2. Upload code
3. Login
4. **Wait 30 seconds**
5. Try to access `/admin`

Expected:
- ↪️ Redirected to `/login` (session expired)

Serial Monitor:
```
Session expired
Access denied - not authenticated
```

**Remember to change back to 1 hour after testing:**
```cpp
#define SESSION_TIMEOUT 3600000  // 1 hour
```

---

### 5️⃣ Test Logout

#### A. Logout Flow

1. Login to admin panel
2. Click **"Logout"** button

Expected:
- ↪️ Redirected to `/login`
- Session cookie cleared (check DevTools)
- Cannot access `/admin` anymore (redirects to login)

Serial Monitor:
```
Logout request
Session destroyed
```

#### B. Verify Session Destroyed

1. After logout, check DevTools cookies
2. Cookie `session` should be:
   - Deleted, OR
   - Max-Age: `0` (expired)

---

### 6️⃣ Test Change Password API

**Using PowerShell:**

```powershell
# First, login to get session cookie
$login = Invoke-WebRequest -Uri "http://172.20.10.2/api/login" `
  -Method POST `
  -Body '{"username":"admin","password":"admin123"}' `
  -ContentType "application/json" `
  -SessionVariable session

# Extract session cookie
$sessionCookie = $session.Cookies.GetCookies("http://172.20.10.2") | Where-Object { $_.Name -eq "session" }

# Change password
Invoke-WebRequest -Uri "http://172.20.10.2/api/change-password" `
  -Method POST `
  -Body '{"current_password":"admin123","new_password":"newpass123"}' `
  -ContentType "application/json" `
  -WebSession $session
```

Expected response:
```json
{
  "success": true,
  "message": "Password changed successfully. Please login again."
}
```

Serial Monitor:
```
--- Change Password Request ---
Password changed successfully
Session destroyed
```

**Verify:**
1. Try login with old password (`admin123`) → ❌ Should fail
2. Try login with new password (`newpass123`) → ✅ Should succeed

---

### 7️⃣ Test Password Validation

#### A. Empty Password

```powershell
POST /api/change-password
Body: {"current_password":"","new_password":"test"}
```

Expected:
```json
{
  "success": false,
  "message": "Current and new password required"
}
```

#### B. Short Password

```powershell
POST /api/change-password
Body: {"current_password":"admin123","new_password":"abc"}
```

Expected:
```json
{
  "success": false,
  "message": "New password must be at least 6 characters"
}
```

#### C. Wrong Current Password

```powershell
POST /api/change-password
Body: {"current_password":"wrongpass","new_password":"newpass123"}
```

Expected:
```json
{
  "success": false,
  "message": "Current password is incorrect"
}
```

---

### 8️⃣ Test Security

#### A. No Session Cookie

```powershell
Invoke-WebRequest -Uri "http://172.20.10.2/api/change-password" `
  -Method POST `
  -Body '{"current_password":"admin123","new_password":"test123"}' `
  -ContentType "application/json"
```

Expected:
- ↪️ Redirect to `/login` (302)
- Password NOT changed

#### B. Invalid Session Token

Edit cookie manually in DevTools:
1. Change `session` value to `invalid_token_12345`
2. Try to access `/admin`

Expected:
- ↪️ Redirected to `/login`
- Session validation failed

---

## ✅ Success Criteria

All tests must pass:

- ✅ Login page loads with beautiful UI
- ✅ Wrong credentials rejected with error message
- ✅ Correct credentials → redirect to admin
- ✅ Protected routes require authentication
- ✅ Session cookie created and validated
- ✅ Session expires after timeout
- ✅ Logout destroys session
- ✅ Password change works with validation
- ✅ Security: unauthenticated requests blocked

---

## 🐛 Common Issues

### Issue 1: "session= undefined" in Cookie

**Cause:** WebServer headers not properly sent

**Fix:** Check `auth.h` line where cookie is set:
```cpp
String cookie = "session=" + currentSession.token + "; Path=/; ...";
server.sendHeader("Set-Cookie", cookie);
```

### Issue 2: Always redirected to login

**Cause:** Cookie not being sent by browser

**Fix:**
1. Check cookie in DevTools
2. Verify `Path=/` is set
3. Try different browser
4. Clear all cookies and retry

### Issue 3: Session timeout not working

**Cause:** `millis()` overflow or SESSION_TIMEOUT too large

**Fix:**
1. Check `config.h` SESSION_TIMEOUT value
2. Verify calculation in `isSessionValid()`

---

## 📊 Test Results Template

```
Date: ___________
Tester: ___________
ESP32 IP: ___________

[ ] Login page loads correctly
[ ] Wrong credentials rejected
[ ] Correct credentials accepted
[ ] Protected routes work
[ ] Session cookie created
[ ] Session timeout works
[ ] Logout destroys session
[ ] Change password works
[ ] Password validation works
[ ] Security checks pass

Notes:
_____________________
_____________________
```

---

## 🎯 Next Steps

After Task 5 passes all tests:
- ✅ Commit changes
- 🚀 Start **Task 6: Admin Dashboard**
  - Statistics display
  - Motor control UI
  - Device info
  - Settings page

---

**File Created:** 2024-11-10  
**Last Updated:** 2024-11-10  
**Version:** 1.0

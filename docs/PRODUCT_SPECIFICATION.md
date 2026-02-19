# Kinemachina SFX Controller
## Product Specification Document

**Version:** 1.0  
**Date:** 2024  
**Document Type:** Product Specification for Market Analysis & Kickstarter  
**Status:** Pre-Launch

---

## Executive Summary

The **Kinemachina SFX Controller** is a professional-grade, WiFi-enabled audio playback and LED effects controller designed for makers, hobbyists, and commercial installations. Built on the ESP32-S3 platform, it combines high-quality audio playback, synchronized LED effects, and web-based control into a single, easy-to-use device.

**Key Value Proposition:**
- **Professional Audio:** 32-bit, 384kHz I2S DAC for studio-quality sound
- **Synchronized Effects:** 48 LED effects (23 strip + 25 matrix) with optional audio-reactive synchronization
- **Dual LED Outputs:** Independent control of LED strips (GPIO 6) and 16x16 matrix panels (GPIO 5)
- **Zero-Code Configuration:** JSON-based setup, no programming required
- **Battle-Ready API:** RESTful API for integration with automation systems
- **Open Source:** Fully open-source hardware and software

---

## Product Overview

### What It Is

A compact, WiFi-enabled controller that plays audio files from an SD card while synchronizing customizable LED effects. Designed for:
- **Interactive Displays:** Museum exhibits, retail displays, themed installations
- **Battle Arenas:** Automated battle systems with camera tracking
- **Home Automation:** Smart lighting with synchronized audio
- **Maker Projects:** Custom effects for cosplay, props, and installations
- **Commercial Installations:** Themed restaurants, escape rooms, entertainment venues

### What It Does

1. **Audio Playback:** Plays MP3, WAV, AAC, M4A, FLAC, OGG files from SD card
2. **Dual LED Control:** Independent control of WS2812B NeoPixel strips (GPIO 6) and matrix panels (GPIO 5) with 48 pre-programmed effects
3. **Strip Effects:** 23 effects optimized for linear LED strips (themed, popular effects, edge-lit disc patterns)
4. **Matrix Effects:** 25 effects designed for 16x16 (256 pixel) panels with 2D coordinate system and battle arena behaviors
5. **Synchronization:** Audio-reactive LED effects that respond to music in real-time
6. **Web Control:** Full-featured web interface with effects grouped by Strip and Matrix types
7. **API Integration:** RESTful JSON API for automation and master-slave systems
8. **Demo Mode:** Automatic playback with random or sequential effects
9. **Effect System:** Custom effects combining audio, LED, or both with looping

---

## Target Markets

### Primary Markets

1. **Maker/Hobbyist Market**
   - Cosplay enthusiasts
   - Prop builders
   - DIY electronics hobbyists
   - Arduino/ESP32 enthusiasts
   - LED art installations

2. **Commercial Entertainment**
   - Escape rooms
   - Themed restaurants
   - Museum exhibits
   - Retail displays
   - Interactive installations

3. **Professional Integrators**
   - Home automation installers
   - AV system integrators
   - Event production companies
   - Custom installation specialists

4. **Educational Market**
   - STEM educators
   - Makerspaces
   - University labs
   - Technical training programs

### Market Size Indicators

- **Maker Market:** 135M+ makers globally (Maker Media)
- **Smart Home Market:** $53.45B by 2025 (Grand View Research)
- **LED Lighting Market:** $108.99B by 2026 (Fortune Business Insights)
- **Escape Room Industry:** $2.3B globally, 50%+ growth rate

---

## Product Features

### Core Features

#### Audio System
- ✅ **High-Quality DAC:** PCM5122 32-bit, 384kHz I2S DAC
- ✅ **Format Support:** MP3, WAV, AAC, M4A, FLAC, OGG
- ✅ **Volume Control:** 0-21 step digital volume control
- ✅ **SD Card Storage:** FAT32 formatted, 8GB+ support
- ✅ **Playback Modes:** Single play, loop, sequential, random

#### LED Effects System
- ✅ **48 Pre-Programmed Effects Total:**
  - **Strip Effects (23):** Themed (7), Popular effects (10), Edge-lit disc effects (6)
  - **Matrix Effects (25):** Battle Arena - Fight Behaviors (8), Battle Arena - Dance Behaviors (8), Panel Effects (9)
- ✅ **Dual LED Outputs:** Independent controllers for strips (GPIO 6) and matrix panels (GPIO 5)
- ✅ **16x16 Panel Support:** Dedicated matrix controller with full 2D grid mode and serpentine wiring support
- ✅ **Grid Drawing Primitives:** Lines, circles, rectangles for custom grid effects
- ✅ **Audio-Reactive Mode:** LED effects sync to audio amplitude (works for both strip and matrix)
- ✅ **Brightness Control:** 0-255 level control (shared between controllers)
- ✅ **NeoPixel Support:** WS2812B and compatible strips/panels
- ✅ **Configurable Count:** Supports 1-1000+ pixels (strips) or 16x16 panels (256 pixels)

#### Control & Interface
- ✅ **Web Interface:** Full-featured HTML5 control panel with LED effects grouped by Strip Effects and Matrix Effects
- ✅ **REST API:** JSON-based HTTP API for automation
- ✅ **MQTT Support:** Event-driven pub/sub messaging for real-time control
  - Automatic status publishing on state changes
  - Health checks every 30 seconds
  - Last Will and Testament (LWT) for online/offline status
  - mDNS broker resolution (supports `.local` hostnames)
  - FreeRTOS queue-based command processing for thread-safe operation
  - Configurable QoS levels for commands and status messages
  - Automatic reconnection with exponential backoff
- ✅ **WiFi Connectivity:** 802.11 b/g/n, easy setup
- ✅ **Configuration File:** JSON-based, no coding required
- ✅ **Settings Persistence:** Automatic save/restore
- ✅ **mDNS/Bonjour:** Automatic broker discovery via .local hostnames

#### Advanced Features
- ✅ **Demo Mode:** Automatic playback with 4 modes (Audio+LED, Audio Only, LED Only, Effects)
- ✅ **Playback Order:** Random or sequential
- ✅ **Effect Categories:** Organize effects (attack, growl, death, damage, victory, idle)
- ✅ **Looping Support:** Continuous effect playback with error protection
- ✅ **Battle Arena API:** Master-slave control via HTTP REST API
- ✅ **MQTT Control:** Event-driven MQTT pub/sub for battle arena and automation
  - Low-latency command processing (<50ms)
  - Request/response correlation via `request_id` field
  - Support for effect categories and named effects
  - Optional parameter overrides (volume, brightness)
- ✅ **Automatic Status Publishing:** Real-time status updates via MQTT topics
  - State change detection (audio, LED, volume, brightness)
  - Separate topics for audio, LED, and health status
  - Retained messages for last known state
- ✅ **FreeRTOS Queues:** Thread-safe command processing with dedicated task

### Technical Specifications

#### Hardware
- **MCU:** ESP32-S3 (Dual-core 240MHz, 16MB Flash, 8MB PSRAM)
- **Audio DAC:** PCM5122 (32-bit, 384kHz, 112dB SNR)
- **Storage:** MicroSD card slot (SPI interface)
- **LED Control:** Dual NeoPixel outputs (WS2812B compatible)
  - **Strip Output (GPIO 6):** 1-1000+ pixels (linear strips)
  - **Matrix Output (GPIO 5):** 16x16 (256 pixels) panels
  - Both outputs operate independently and can be used simultaneously
- **Connectivity:** WiFi 802.11 b/g/n, USB-C for programming
- **Power:** 5V USB-C input, 3A recommended (5A+ for large panels)
- **Dimensions:** TBD (PCB design pending)
- **Operating Temperature:** 0°C to 70°C

#### Software
- **Framework:** Arduino/ESP-IDF
- **Libraries:** ESP32-audioI2S, Adafruit NeoPixel, ESPAsyncWebServer, AsyncMqttClient
- **Protocols:** HTTP/REST, MQTT, JSON
- **File System:** FAT32
- **Update Method:** USB-C (future: OTA)
- **Task Management:** FreeRTOS with queues and dedicated tasks

#### Performance
- **Audio Latency:** <50ms
- **LED Update Rate:** 50 FPS (strips and grids)
- **Grid Rendering:** Optimized 2D coordinate system with serpentine wiring support (matrix controller only)
- **WiFi Range:** 30-50m (indoor)
- **Concurrent Connections:** 10+ web clients
- **SD Card Speed:** Class 10 recommended
- **FreeRTOS:** Queue-based inter-task communication for smooth operation

---

## Use Cases & Applications

### 1. Kinemachina Battle Arena System
**Target:** Hobbyists, makers, entertainment venues
**Application:** Automated battle system with camera tracking
- Master computer tracks targets with camera
- Triggers effects via MQTT or HTTP REST API based on battle events
- **MQTT Advantages:** Event-driven pub/sub messaging for low-latency response (<50ms)
- Automatic status publishing for real-time feedback and system monitoring
- Health checks enable proactive monitoring and fault detection
- Last Will and Testament (LWT) provides instant offline detection
- Request/response correlation enables reliable command tracking
- Synchronized audio and LED effects for immersive experience
- Categories: attack, growl, damage, death, victory
- Supports multiple controllers via topic-based addressing

**Market Size:** Niche but passionate (creative builders, makers)

### 2. Interactive Museum Exhibits
**Target:** Museums, science centers, educational institutions  
**Application:** Touch-triggered audio and visual effects
- Web interface for staff control
- Demo mode for unattended operation
- Multiple effect categories for different exhibit themes
- Professional audio quality for public spaces

**Market Size:** 35,000+ museums globally

### 3. Themed Restaurants & Escape Rooms
**Target:** Entertainment venues, escape room operators  
**Application:** Ambient effects and triggered events
- Synchronized audio/LED for room themes
- Remote control via web interface
- Demo mode for continuous ambiance
- Easy configuration without programming

**Market Size:** 50,000+ escape rooms globally, growing 50%+ annually

### 4. Home Automation & Smart Lighting
**Target:** Smart home enthusiasts, DIY home automation  
**Application:** Audio-synchronized lighting
- Audio-reactive LED effects
- **MQTT Integration:** Native support for Home Assistant, Node-RED, OpenHAB
- Event-driven control perfect for automation rules and scenes
- Automatic status publishing enables real-time dashboards
- REST API for home automation integration
- Web interface for easy control
- Custom effects for parties and events
- mDNS broker discovery simplifies setup in local networks

**Market Size:** 135M+ smart home devices shipped annually

### 5. Cosplay & Prop Building
**Target:** Cosplayers, prop makers, convention exhibitors  
**Application:** Animated props with sound and light
- Portable, battery-powered operation
- Easy effect configuration
- Multiple effect categories
- Professional audio quality

**Market Size:** 20M+ cosplayers globally, $4.6B prop market

### 6. LED Art Installations
**Target:** Artists, galleries, public art projects  
**Application:** Interactive light installations
- 48 pre-programmed effects (23 strip effects + 25 matrix effects)
- Audio-reactive synchronization
- Edge-lit disc effects for circular displays
- 16x16 panel support for grid-based art
- Grid drawing primitives for custom patterns
- Web-based control for gallery management

**Market Size:** $65B+ global art market

---

## Competitive Analysis

### Direct Competitors

| Product | Price | Key Features | Our Advantage |
|---------|-------|--------------|---------------|
| **Raspberry Pi + HATs** | $75-150 | Flexible, requires coding | Zero-code configuration, built-in effects |
| **Adafruit Audio FX** | $20-40 | Audio only, limited | Audio + LED, WiFi, web interface |
| **WLED Controllers** | $10-30 | LED only | Audio + LED, synchronized effects |
| **Commercial AV Controllers** | $500-2000 | Professional, expensive | Affordable, open-source, maker-friendly |

### Competitive Advantages

1. **Integrated Solution:** Audio + LED in one device (competitors typically require separate systems)
2. **Zero-Code Setup:** JSON configuration vs. programming required
3. **Audio-Reactive:** Built-in synchronization (most competitors lack this)
4. **Battle Arena API:** Unique feature for automated systems with both HTTP REST and MQTT support
5. **MQTT Integration:** Event-driven pub/sub messaging with automatic status publishing (rare in maker products)
6. **Grid Panel Support:** 16x16 panel support with 25 specialized battle arena effects (unique in market)
7. **Open Source:** Full hardware/software open-source (competitive advantage in maker market)
8. **Professional Audio:** 32-bit DAC vs. typical 16-bit in competitors
9. **Affordable:** Lower cost than commercial solutions
10. **FreeRTOS Optimized:** Queue-based architecture for smooth, reliable operation

### Market Positioning

**Position:** Premium maker product, affordable professional solution

- **vs. DIY Solutions:** Easier to use, professional quality
- **vs. Commercial Solutions:** More affordable, open-source, customizable
- **vs. Single-Function Products:** More features, better integration

---

## Pricing Strategy

### Cost Analysis (Estimated)

#### Bill of Materials (BOM)
- ESP32-S3 Module: $8-12
- PCM5122 DAC: $3-5
- Power Regulators: $1-2
- Passives & Connectors: $3-5
- PCB & Assembly: $5-8
- **Total BOM:** ~$20-32 per unit

#### Additional Costs
- Packaging: $2-3
- Shipping: $3-5
- Marketing/Support: 20-30% margin
- **Total Cost:** ~$30-45 per unit

### Pricing Tiers

#### Tier 1: Early Bird (Kickstarter)
- **Price:** $49-59
- **Target:** First 100-200 backers
- **Margin:** ~40-50%
- **Value:** 30-40% discount vs. retail

#### Tier 2: Super Early Bird
- **Price:** $59-69
- **Target:** Next 200-300 backers
- **Margin:** ~45-55%
- **Value:** 20-30% discount vs. retail

#### Tier 3: Kickstarter Regular
- **Price:** $69-79
- **Target:** Remaining Kickstarter backers
- **Margin:** ~50-60%
- **Value:** 10-20% discount vs. retail

#### Tier 4: Retail (Post-Kickstarter)
- **Price:** $89-99
- **Target:** General market
- **Margin:** ~60-70%
- **Value:** Standard retail pricing

### Bundle Options

#### Starter Bundle
- Controller + 8 NeoPixel strip + SD card + USB cable
- **Price:** $99-119 (Kickstarter), $139-159 (Retail)
- **Value Add:** $20-30 savings vs. buying separately

#### Pro Bundle
- Controller + 30 NeoPixel strip + 16GB SD card + USB cable + Audio amplifier
- **Price:** $149-169 (Kickstarter), $199-219 (Retail)
- **Value Add:** $40-50 savings

#### Battle Arena Bundle
- Controller + 16x16 WS2812B panel (256 pixels) + 16GB SD card + USB cable + Audio amplifier
- **Price:** $199-229 (Kickstarter), $279-299 (Retail)
- **Value Add:** $60-80 savings
- **Target:** Battle arena builders, makers with grid displays

#### Developer Bundle
- Controller + Accessories + Documentation + Early API access
- **Price:** $199-249
- **Target:** Professional integrators, developers

### Price Justification

**vs. DIY (Raspberry Pi + Components):**
- DIY Cost: $75-100 + time investment
- Our Product: $69-99, ready to use
- **Value:** Time savings, integrated solution

**vs. Commercial Solutions:**
- Commercial: $500-2000
- Our Product: $89-99
- **Value:** 80-95% cost savings

**vs. Single-Function Products:**
- Audio Controller: $40-60
- LED Controller: $20-40
- Total: $60-100
- Our Product: $69-99 (both in one)
- **Value:** Better integration, synchronized effects

---

## Kickstarter Campaign Strategy

### Campaign Goals

#### Funding Goal
- **Minimum Viable:** $25,000 (500 units @ $50 avg)
- **Stretch Goal 1:** $50,000 (1,000 units)
- **Stretch Goal 2:** $100,000 (2,000 units)
- **Stretch Goal 3:** $200,000 (4,000 units)

#### Timeline
- **Campaign Duration:** 30-45 days
- **Production Timeline:** 3-4 months post-campaign
- **Shipping:** 4-5 months post-campaign

### Campaign Structure

#### Rewards Tiers

1. **$1 - Supporter**
   - Thank you in credits
   - Early access to documentation
   - Community access

2. **$49 - Early Bird (Limited 100)**
   - 1x Kinemachina Controller
   - USB-C cable
   - Digital documentation
   - Early shipping

3. **$59 - Super Early Bird (Limited 200)**
   - 1x Kinemachina Controller
   - USB-C cable
   - Digital documentation

4. **$69 - Kickstarter Special**
   - 1x Kinemachina Controller
   - USB-C cable
   - Digital documentation
   - Sticker pack

5. **$99 - Starter Bundle**
   - 1x Kinemachina Controller
   - 8 NeoPixel strip
   - 8GB SD card
   - USB-C cable
   - Digital documentation

6. **$149 - Pro Bundle**
   - 1x Kinemachina Controller
   - 30 NeoPixel strip
   - 16GB SD card
   - USB-C cable
   - Audio amplifier module
   - Digital documentation

7. **$199 - Developer Bundle**
   - 1x Kinemachina Controller
   - All accessories
   - Early API access
   - Priority support
   - Developer documentation

8. **$299 - Double Pack**
   - 2x Kinemachina Controllers
   - All accessories x2
   - Multi-zone setup guide

### Stretch Goals

1. **$50,000 - Enhanced Documentation**
   - Video tutorials
   - Advanced API documentation
   - Community forum

2. **$100,000 - Additional Effects** ✅ ACHIEVED
   - 25 matrix effects (8 fight + 8 dance + 9 panel effects)
   - Dedicated matrix controller with 16x16 panel support
   - Grid drawing primitives
   - Dual LED output architecture (strip + matrix)
   - Effect editor tool (future)
   - Preset library (future)

3. **$200,000 - Display Module**
   - 2" ST7789 color display add-on
   - Status screen features
   - Touch interface option

4. **$300,000 - Mobile App**
   - iOS/Android app
   - Remote control
   - Effect preview

### Marketing Angles

#### Primary Messaging

1. **"Professional Audio + LED Control, Zero Coding Required"**
   - Target: Makers who want professional results without programming
   - Pain Point: Complex setup, coding requirements
   - Solution: JSON configuration, web interface

2. **"Battle-Ready for Your Arena"**
   - Target: Enthusiasts, battle arena builders
   - Pain Point: Need for automated, synchronized effects with visual displays
   - Solution: REST API, dual LED outputs (strip + matrix), 16x16 panel support with 25 specialized matrix effects

3. **"Open Source, Professional Quality"**
   - Target: Open-source enthusiasts, professional integrators
   - Pain Point: Proprietary solutions, vendor lock-in
   - Solution: Full open-source hardware/software

4. **"Affordable Alternative to $500+ Commercial Solutions"**
   - Target: Small businesses, escape rooms, museums
   - Pain Point: High cost of commercial AV controllers
   - Solution: 80-90% cost savings

#### Marketing Channels

1. **Maker Communities**
   - Hackaday, Instructables, Reddit (r/arduino, r/esp32, r/led)
   - Maker Faires, local maker groups
   - YouTube makers and influencers

2. **Creative/Maker Communities**
   - Cosplay communities
   - Prop building forums
   - DIY enthusiast groups

3. **Commercial Markets**
   - Escape room operator groups
   - Museum technology forums
   - AV integrator networks

4. **Tech Media**
   - Hackaday, Make Magazine, Adafruit Blog
   - Electronics blogs and YouTube channels
   - Kickstarter discovery

### Campaign Assets Needed

1. **Video (2-3 minutes)**
   - Product demo
   - Use case examples
   - Battle arena system demo with 16x16 panel effects
   - Grid effects showcase (fight and dance behaviors)
   - Maker testimonials (if available)

2. **Images**
   - Product photos (multiple angles)
   - In-use photos (battle arena, installations)
   - Technical diagrams
   - Comparison charts

3. **Documentation**
   - Technical specifications
   - API documentation preview
   - Setup guide preview
   - Use case examples

4. **Social Proof**
   - Beta tester testimonials
   - Early adopter reviews
   - Community engagement

---

## Market Analysis

### Total Addressable Market (TAM)

**Global Electronics Market:** $1.5T+  
**Maker Market:** $135M+ makers globally  
**Smart Home Market:** $53.45B by 2025  
**LED Lighting Market:** $108.99B by 2026

### Serviceable Addressable Market (SAM)

**Target Segments:**
- Makers/Hobbyists: 10M+ (active electronics hobbyists)
- Escape Rooms: 50,000+ globally
- Museums: 35,000+ globally
- Themed Restaurants: 100,000+ globally
- Cosplayers: 20M+ globally

**Estimated SAM:** $500M - $1B

### Serviceable Obtainable Market (SOM)

**Year 1 Targets:**
- Kickstarter: 500-2,000 units
- Post-Kickstarter: 1,000-3,000 units
- **Total Year 1:** 1,500-5,000 units
- **Revenue:** $100K - $500K

**Year 2-3 Targets:**
- Retail channels: 5,000-10,000 units/year
- Direct sales: 2,000-5,000 units/year
- **Total:** 7,000-15,000 units/year
- **Revenue:** $600K - $1.5M/year

### Market Trends

1. **Growing Maker Movement:** 135M+ makers, $29B market
2. **Smart Home Adoption:** 20%+ annual growth
3. **LED Market Growth:** 13%+ CAGR
4. **Escape Room Growth:** 50%+ annual growth
5. **Open Source Hardware:** Growing acceptance in commercial markets

### Market Risks

1. **Competition:** Established players may copy features
2. **Technology Changes:** New platforms may emerge
3. **Economic Downturn:** Discretionary spending may decrease
4. **Supply Chain:** Component shortages, price increases

### Market Opportunities

1. **Partnerships:** Escape room suppliers, museum tech companies
2. **OEM Opportunities:** White-label for integrators
3. **Educational Market:** STEM curriculum integration
4. **International Expansion:** Global maker community

---

## Go-to-Market Strategy

### Phase 1: Pre-Launch (Months 1-3)
- Finalize PCB design
- Beta testing with select makers
- Build community (Discord, Reddit, forums)
- Create marketing assets
- Secure early testimonials

### Phase 2: Kickstarter Launch (Month 4)
- Launch campaign
- Social media blitz
- Maker community outreach
- Press releases to tech media
- Influencer partnerships

### Phase 3: Production (Months 5-8)
- Finalize manufacturing partners
- Component sourcing
- PCB fabrication and assembly
- Quality testing
- Packaging design

### Phase 4: Fulfillment (Months 9-10)
- Kickstarter fulfillment
- Customer support setup
- Documentation finalization
- Community building

### Phase 5: Retail Launch (Month 11+)
- E-commerce store launch
- Retail partnerships
- Marketing campaigns
- Product iterations based on feedback

---

## Success Metrics

### Kickstarter Campaign
- **Funding Goal:** $25,000 minimum
- **Backer Count:** 500+ backers
- **Conversion Rate:** 2-5% (visitors to backers)
- **Average Pledge:** $60-80

### Post-Campaign
- **Customer Satisfaction:** 4.5+ stars
- **Repeat Purchase Rate:** 20-30%
- **Community Growth:** 1,000+ members
- **API Adoption:** 100+ developers

### Long-Term
- **Market Share:** Top 3 in maker audio+LED controllers
- **Revenue Growth:** 50%+ year-over-year
- **Community:** 10,000+ active users
- **Partnerships:** 5+ commercial integrators

---

## Risk Assessment

### Technical Risks
- **Component Availability:** Mitigation - Multiple supplier options
- **Firmware Bugs:** Mitigation - Extensive beta testing
- **Hardware Issues:** Mitigation - Professional PCB design review

### Market Risks
- **Low Demand:** Mitigation - Strong pre-launch validation
- **Competition:** Mitigation - Unique features, open-source advantage
- **Pricing Pressure:** Mitigation - Value-based pricing, bundles

### Operational Risks
- **Manufacturing Delays:** Mitigation - Buffer time, multiple suppliers
- **Quality Issues:** Mitigation - Quality control processes
- **Support Burden:** Mitigation - Comprehensive documentation, community

---

## Appendix

### Technical Documentation
- Full BOM (see BOM.md)
- PCB schematics (pending)
- API documentation (see codebase)
- Setup guides (see README.md)

### Legal Considerations
- Open-source license (MIT/Apache recommended)
- Hardware license (CERN OHL or similar)
- Trademark protection (Kinemachina Controller name)
- Patents (if applicable)

### Support & Community
- Documentation website
- Community forum/Discord
- GitHub repository
- Video tutorials
- Email support

---

## Revision History

| Version | Date | Changes | Author |
|---------|------|---------|--------|
| 1.0 | 2024 | Initial specification | - |

---

**Document Status:** Draft for Review  
**Next Steps:** Market validation, pricing refinement, Kickstarter preparation

plugins {
    id("com.android.application")
}

android {
    namespace = "com.brunogamacatao.knightmare"
    compileSdk = 36
    ndkVersion = "27.2.12479018"

    defaultConfig {
        applicationId = "com.brunogamacatao.knightmare"
        minSdk = 24
        targetSdk = 36
        versionCode = 1
        versionName = "1.0.0"

        externalNativeBuild {
            cmake {
                // Same CMakeLists.txt used by the desktop build (see ../../CMakeLists.txt) - just
                // pointed at PLATFORM=Android, which switches it to build a shared library linked
                // against a PLATFORM_ANDROID raylib instead of a desktop executable.
                arguments += listOf("-DPLATFORM=Android")
                abiFilters += listOf("arm64-v8a", "armeabi-v7a", "x86_64")
            }
        }
    }

    // Points straight at the project-root CMakeLists.txt shared with the desktop build, rather
    // than duplicating it under app/src/main/cpp - there is nothing Android-specific to add: no
    // Java/Kotlin sources exist in this module (this is a pure NativeActivity app, see
    // AndroidManifest.xml's android:hasCode="false" and android.app.lib_name meta-data).
    externalNativeBuild {
        cmake {
            path = file("../../CMakeLists.txt")
        }
    }

    sourceSets {
        getByName("main") {
            manifest.srcFile("src/main/AndroidManifest.xml")
            assets.srcDirs("src/main/assets")
            res.srcDirs("src/main/res")
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    buildTypes {
        release {
            isMinifyEnabled = false
        }
    }
}
